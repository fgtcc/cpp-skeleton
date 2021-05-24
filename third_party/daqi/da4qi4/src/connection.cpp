#include "daqi/connection.hpp"

#include <ctime>
#include <ostream>

#include "daqi/utilities/string_utilities.hpp"

#include "daqi/def/log_def.hpp"
#include "daqi/def/boost_def.hpp"
#include "daqi/def/asio_def.hpp"
#include "daqi/application.hpp"

#include "daqi/websocket/connection_websocket.hpp"
#include "daqi/websocket/context_websocket.hpp"

namespace da4qi4
{

Connection::Connection(IOC& ioc, size_t ioc_index)
    : _with_ssl(false), _socket_ptr(new net_detail::Socket(ioc))
    , _ioc_index(ioc_index)
    , _parser(new http_parser)
    , _mp_parser(nullptr, &::multipart_parser_free)
{
    this->init_parser_setting();
    this->init_parser();
}

Connection::Connection(IOC& ioc, size_t ioc_index, boost::asio::ssl::context& ssl_ctx)
    : _with_ssl(true), _socket_ptr(new net_detail::SocketWithSSL(ioc, ssl_ctx))
    , _ioc_index(ioc_index)
    , _parser(new http_parser)
    , _mp_parser(nullptr, &::multipart_parser_free)
{
    this->init_parser_setting();
    this->init_parser();
}

ApplicationPtr Connection::GetApplication()
{
    return _app;
}

void Connection::init_parser()
{
    llhttp_init(_parser.get(), HTTP_REQUEST, &_parser_setting);
    _parser->data = this;
}

void Connection::init_parser_setting()
{
    llhttp_settings_init(&_parser_setting);

    _parser_setting.on_message_begin = &Connection::on_message_begin;
    _parser_setting.on_message_complete = &Connection::on_message_complete;
    _parser_setting.on_headers_complete = &Connection::on_headers_complete;
    _parser_setting.on_header_field = &Connection::on_header_field;
    _parser_setting.on_header_value = &Connection::on_header_value;
    _parser_setting.on_url = &Connection::on_url;
    _parser_setting.on_body = &Connection::on_body;
}

void Connection::Start()
{
    if (!_with_ssl)
    {
        StartRead();
        return;
    }

    this->do_handshake();
}

void Connection::StartRead()
{
    this->do_read();
}

void Connection::Stop()
{
    this->do_close();
}

void Connection::StartWrite()
{
    if (!_response.IsChunked())
    {
        this->do_write();
    }
    else
    {
        this->do_write_header_for_chunked();
    }
}

void Connection::update_request_after_header_parsed()
{
    if (!_url_buffer.empty())
    {
        _request.ParseUrl(std::move(_url_buffer));
    }

    _request.SetFlags(_parser->flags);

    if (_parser->flags &  F_CONTENT_LENGTH)
    {
        _request.SetContentLength(_parser->content_length);
    }

    _request.MarkKeepAlive(llhttp_should_keep_alive(_parser.get()));
    _request.MarkUpgrade(_parser->upgrade);
    _request.SetMethod(_parser->method);
    _request.SetVersion(_parser->http_major, _parser->http_minor);
    _response.SetVersion(_parser->http_major, _parser->http_minor);
    try_commit_reading_request_header();
    _request.TransferHeadersToCookies();
    _request.ParseContentType();
}

void Connection::update_request_url_after_app_resolve()
{
    assert(_app != nullptr);
    _request.ApplyApplication(_app->GetUrlRoot());
}

void Connection::try_commit_reading_request_header()
{
    bool have_a_uncommit_header = _reading_header_part == Connection::header_value_part
                                  && !_reading_header_field.empty();

    if (have_a_uncommit_header)
    {
        _request.AppendHeader(std::move(_reading_header_field), std::move(_reading_header_value));
        _reading_header_part = Connection::header_none_part;
    }
}

bool is_100_continue(Request& req)
{
    auto value = req.TryGetHeader("Expect");
    return (value && Utilities::iEquals(*value, "100-continue"));
}

void Connection::process_100_continue_request()
{
    assert(is_100_continue(_request));
    _response.ReplyContinue();
    this->StartWrite();
}

void Connection::process_app_no_found()
{
    _response.ReplyNofound();
    this->StartWrite();
}

void Connection::process_too_large_size_upload()
{
    _response.ReplyPayloadTooLarge();
    this->StartWrite();
}

void Connection::try_init_multipart_parser()
{
    assert(_request.IsMultiPart());

    if (!_request.GetMultiPartBoundary().empty()) //boundary is unset on some bad request.
    {
        init_multipart_parser(_request.GetMultiPartBoundary());
    }
}

bool Connection::try_route_application()
{
    if (!_request.GetUrl().schema.empty() || !_request.GetUrl().host.empty())
    {
        return false;
    }

    _app = AppMgr().FindByURL(_request.GetUrl().path);
    return _app != nullptr;
}

int Connection::on_headers_complete(http_parser* parser)
{
    Connection* cnt = static_cast<Connection*>(parser->data);

    cnt->update_request_after_header_parsed();  //_url parsed!

    cnt->_read_complete = Connection::read_header_complete;

    if (!cnt->try_route_application())
    {
        cnt->process_app_no_found();
        return -1;
    }

    cnt->update_request_url_after_app_resolve();

    if ((cnt->_request.GetContentLength() / 1024) > cnt->_app->GetUpoadMaxSizeLimitKB())
    {
        log::Server()->debug("Host {} : {} > {} KB. {}."
                             , cnt->GetRequest().GetHost()
                             , cnt->_request.GetContentLength()
                             , cnt->_app->GetUpoadMaxSizeLimitKB()
                             , cnt->_app->GetName());
        cnt->process_too_large_size_upload();
        return -1;
    }

    if (is_100_continue(cnt->_request))
    {
        cnt->process_100_continue_request(); //async write
    }
    else if (cnt->_request.IsMultiPart())
    {
        cnt->try_init_multipart_parser();
    }

    return HPE_OK;
}

int Connection::on_message_begin(http_parser* parser)
{
    Connection* cnt = static_cast<Connection*>(parser->data);
    cnt->_read_complete = Connection::read_none_complete;
    cnt->_body_buffer.clear();

    return HPE_OK;
}

int Connection::on_message_complete(http_parser* parser)
{
    Connection* cnt = static_cast<Connection*>(parser->data);
    cnt->_request.SetBody(std::move(cnt->_body_buffer));
    cnt->_read_complete = Connection::read_message_complete;

    return HPE_OK;
}

int Connection::on_header_field(http_parser* parser, char const* at, size_t length)
{
    Connection* cnt = static_cast<Connection*>(parser->data);
    cnt->try_commit_reading_request_header();

    if (length > 0)
    {
        cnt->_reading_header_part = Connection::header_field_part;
        cnt->_reading_header_field.append(at, length);
    }

    return HPE_OK;
}

int Connection::on_header_value(http_parser* parser, char const* at, size_t length)
{
    Connection* cnt = static_cast<Connection*>(parser->data);

    if (cnt->_reading_header_part == Connection::header_field_part
        && !cnt->_reading_header_value.empty())
    {
        cnt->_reading_header_value.clear();
    }

    if (length > 0)
    {
        cnt->_reading_header_value.append(at, length);
    }

    if (cnt->_reading_header_part != Connection::header_value_part)
    {
        cnt->_reading_header_part = Connection::header_value_part;
    }

    return HPE_OK;
}


int Connection::on_url(http_parser* parser, char const* at, size_t length)
{
    Connection* cnt = static_cast<Connection*>(parser->data);
    cnt->_url_buffer.append(at, length);

    return HPE_OK;
}

void Connection::try_fix_multipart_bad_request_without_boundary()
{
    assert(!_mp_parser);

    if (_body_buffer.find("--") == 0)
    {
        constexpr int const length_of_boundary_start_flag = 2;
        std::string::size_type endln_pos = _body_buffer.find("\r\n", length_of_boundary_start_flag);

        if (endln_pos != std::string::npos)
        {
            std::string boundary = _body_buffer.substr(length_of_boundary_start_flag
                                                       , endln_pos - length_of_boundary_start_flag);
            std::string::size_type len = boundary.size();

            if (len > 2 && boundary[len - 2] == '-' && boundary[len - 1] == '-')
            {
                len -= 2;
            }

            _request.SetMultiPartBoundary(boundary.c_str(), len);
            init_multipart_parser(_request.GetMultiPartBoundary());
        }
    }
}

Connection::MultpartParseStatus Connection::do_multipart_parse()
{
    assert(_request.IsMultiPart());

    if (!_mp_parser)
    {
        try_fix_multipart_bad_request_without_boundary();
    }

    if (!_mp_parser)
    {
        return mp_cannot_init;
    }

    size_t parsed_bytes = multipart_parser_execute(_mp_parser.get(), _body_buffer.c_str(), _body_buffer.length());

    if (parsed_bytes != _body_buffer.length())
    {
        return mp_parse_fail;
    }

    _body_buffer.clear(); //body -> multi parts
    return mp_parsing;
}

int Connection::on_body(http_parser* parser, char const* at, size_t length)
{
    Connection* cnt = static_cast<Connection*>(parser->data);

    size_t upload_max_size_limit_kb = (cnt->_app) ? cnt->_app->GetUpoadMaxSizeLimitKB() : 15 * 1024;

    size_t total_byte_kb = (cnt->_body_buffer.size() + length) / 1024;

    if (total_byte_kb > upload_max_size_limit_kb)
    {
        cnt->process_too_large_size_upload();
        return -1;
    }

    cnt->_body_buffer.append(at, length);

    if (cnt->_request.IsMultiPart())
    {
        MultpartParseStatus status = cnt->do_multipart_parse();

        switch (status)
        {
            case mp_cannot_init:
            case mp_parsing:
                break;

            case   mp_parse_fail:
                log::Server()->error("Parse multi-part data fail.");
                return -1;
        }
    }

    return HPE_OK;
}

int Connection::on_multipart_header_field(multipart_parser* parser, char const* at, size_t length)
{
    Connection* cnt = static_cast<Connection*>(multipart_parser_get_data(parser));
    cnt->_multipart_parse_part = mp_parse_header_field;

    if (cnt->_reading_header_part == header_value_part && !cnt->_reading_header_field.empty())
    {
        cnt->_reading_part.AppendHeader(std::move(cnt->_reading_header_field)
                                        , std::move(cnt->_reading_header_value));
        cnt->_reading_header_part = header_field_part;
    }

    cnt->_reading_header_field.append(at, length);

    return 0;
}

int Connection::on_multipart_header_value(multipart_parser* parser, char const* at, size_t length)
{
    Connection* cnt = static_cast<Connection*>(multipart_parser_get_data(parser));
    cnt->_multipart_parse_part = mp_parse_header_value;

    if (cnt->_reading_header_part == Connection::header_field_part
        && !cnt->_reading_header_value.empty())
    {
        cnt->_reading_header_value.clear();
    }

    if (length > 0)
    {
        cnt->_reading_header_value.append(at, length);
    }

    if (cnt->_reading_header_part != Connection::header_value_part)
    {
        cnt->_reading_header_part = Connection::header_value_part;
    }

    return 0;
}

int Connection::on_multipart_headers_complete(multipart_parser* parser)
{
    Connection* cnt = static_cast<Connection*>(multipart_parser_get_data(parser));
    cnt->_multipart_parse_part = mp_parse_headers_complete;

    if (cnt->_reading_header_part == header_value_part && !cnt->_reading_header_field.empty())
    {
        cnt->_reading_part.AppendHeader(std::move(cnt->_reading_header_field)
                                        , std::move(cnt->_reading_header_value));
        cnt->_reading_header_part = header_none_part;
    }

    return 0;
}

int Connection::on_multipart_data_begin(multipart_parser* parser)
{
    Connection* cnt = static_cast<Connection*>(multipart_parser_get_data(parser));
    cnt->_multipart_parse_part = mp_parse_data_begin;

    cnt->_reading_header_part = header_none_part;
    cnt->_reading_header_field.clear();
    cnt->_reading_header_value.clear();
    cnt->_reading_part_buffer.clear();

    return 0;
}

int Connection::on_multipart_data(multipart_parser* parser, char const* at, size_t length)
{
    Connection* cnt = static_cast<Connection*>(multipart_parser_get_data(parser));
    cnt->_multipart_parse_part = mp_parse_data;
    cnt->_reading_part_buffer.append(at, length);

    return 0;
}

int Connection::on_multipart_data_end(multipart_parser* parser)
{
    Connection* cnt = static_cast<Connection*>(multipart_parser_get_data(parser));
    cnt->_multipart_parse_part = mp_parse_data_end;

    cnt->_reading_part.SetData(std::move(cnt->_reading_part_buffer));
    cnt->_request.AddMultiPart(std::move(cnt->_reading_part));

    return 0;
}

int Connection::on_multipart_body_end(multipart_parser* parser)
{
    Connection* cnt = static_cast<Connection*>(multipart_parser_get_data(parser));
    cnt->_multipart_parse_part = mp_parse_body_end;

    return 0;
}

void Connection::init_multipart_parser(std::string const& boundary)
{
    if (!_mp_parser_setting || !_mp_parser)
    {
        if (!_mp_parser_setting)
        {
            _mp_parser_setting.reset(new multipart_parser_settings);

            _mp_parser_setting->on_part_data_begin = &Connection::on_multipart_data_begin;
            _mp_parser_setting->on_part_data = &Connection::on_multipart_data;
            _mp_parser_setting->on_part_data_end = &Connection::on_multipart_data_end;
            _mp_parser_setting->on_header_field = &Connection::on_multipart_header_field;
            _mp_parser_setting->on_header_value = &Connection::on_multipart_header_value;
            _mp_parser_setting->on_headers_complete = &Connection::on_multipart_headers_complete;
            _mp_parser_setting->on_body_end = &Connection::on_multipart_body_end;
        }

        if (_mp_parser)
        {
            this->free_multipart_parser(will_free_mp_parser);
        }

        std::string boundary_with_prefix("--" + boundary);
        _mp_parser.reset(::multipart_parser_init(boundary_with_prefix.c_str(), _mp_parser_setting.get()));
        multipart_parser_set_data(_mp_parser.get(), this);
    }
}

void Connection::free_multipart_parser(mp_free_flag flag)
{
    if (flag & will_free_mp_setting) //reuse if created
    {
        _mp_parser_setting.reset();
    }

    if (_mp_parser && (flag & will_free_mp_parser))
    {
        _mp_parser.reset(); //will call ::multipart_parser_free()
    }
}

void Connection::do_close()
{
    errorcode ec;
    _socket_ptr->close(ec);

    if (ec)
    {
        log::Server()->warn("Socket close exception. {}", ec.message());
    }
}

void Connection::do_handshake()
{
    assert(this->_with_ssl);

    auto socket_withssl = dynamic_cast<net_detail::SocketWithSSL*>(_socket_ptr.get());
    assert(socket_withssl != nullptr);

    auto& stream = socket_withssl->get_stream();

    auto self(this->shared_from_this());

    stream.async_handshake(boost::asio::ssl::stream_base::server
                           , [self](errorcode const & ec)
    {
        if (ec)
        {
            log::Server()->error("Socket handshake on SSL fail. {}", ec.message());
            return;
        }

        self->StartRead();
    });
}

void Connection::do_read()
{
    auto self(shared_from_this());

    _socket_ptr->async_read_some(_buffer, [self, this](errorcode ec
                                                       , std::size_t bytes_transferred)
    {
        if (ec)
        {
            if (ec != boost::asio::error::eof)
            {
                log::Server()->error("Client connection closed not graceful. {}", ec.message());
            }

            return;
        }

        auto parsed_errno = llhttp_execute(_parser.get(), _buffer.data(), bytes_transferred);

        if (parsed_errno != HPE_OK && parsed_errno != HPE_PAUSED_UPGRADE)
        {
            return;
        }

        if (_read_complete != read_message_complete)
        {
            do_read();
            return;
        }

        free_multipart_parser(will_free_mp_parser);

        assert((_app != nullptr) && "MUST HAVE A APPLICATION AFTER REQUEST READ MESSAGE COMPLETED.");

        if (_parser->upgrade)
        {
            do_upgrade();
            return;
        }

        if (_request.IsFormUrlEncoded())
        {
            _request.ParseFormUrlEncodedData();
        }
        else if (_request.IsFormData())
        {
            auto const& options = _app->GetUploadFileSaveOptions();
            std::string dir = _app->GetUploadRoot().native();
            _request.TransferMultiPartsToFormData(options, dir);
        }

        _response.SetCharset(_app->GetDefaultCharset());
        ContextIMP::Make(shared_from_this())->Start(); //cnt -> app -> ctx
    });
}

void Connection::do_upgrade()
{
    assert(_request.IsUpgrade());

    auto ws_upgrade_value = _request.GetHeader("Upgrade");

    if (ws_upgrade_value.empty() || !Utilities::iEquals(ws_upgrade_value, "websocket"))
    {
        log::Server()->warn("Bad websocket upgrade header value {}. {}. connection will close."
                            , ws_upgrade_value, _request.GetUrl().full);
        return;
    }

    auto ws_key_value = _request.GetHeader("Sec-WebSocket-Key");

    if (ws_key_value.empty())
    {
        log::Server()->warn("No found websocket Sec-WebSocket-Key. {}. connection will close."
                            , _request.GetUrl().full);
        return;
    }

    auto ws_evts_handler = _app->CreateWebSocketHandler(_request.GetUrl().full, UrlFlag::url_full_path);

    if (!ws_evts_handler)
    {
        log::Server()->warn("No found websocket event handler. {}. connection will close."
                            , _request.GetUrl().full);
        return;
    }

    auto ws_connection = Websocket::Connection::Create(_ioc_index
                                                       , _socket_ptr.release()
                                                       , std::move(this->_request.GetUrl())
                                                       , std::move(this->_request.GetHeader())
                                                       , std::move(this->_request.GetCookies())
                                                       , ws_evts_handler.release());

    auto ctx = Websocket::ContextIMP::Create(ws_connection, _app);

    if (!ws_connection->GetEventHandler()->OnOpen(ctx))
    {
        return;
    }

    if (ws_connection->GetID().empty())
    {
        ws_connection->SetID(Utilities::GetUUID(_app->GetName()));
    }

    _app->AddWebSocketConnection(ws_connection);
    ws_connection->Start(ctx, ws_key_value);

    return;
}

void Connection::prepare_response_headers_about_connection()
{
    auto v = _request.GetVersion();
    bool keepalive = _request.IsKeepAlive();

    if (v.first < 1 || (v.first == 1 && v.second == 0))
    {
        _response.SetVersion(1, 0);

        if (keepalive)
        {
            _response.MarkKeepAlive();
        }
    }
    else
    {
        _response.SetVersion(1, 1);

        if (!keepalive)
        {
            _response.MarkClose();
        }
    }
}

void Connection::do_write()
{
    prepare_response_headers_about_connection();

    std::ostream os(&_write_buffer);
    os << _response;

    ConnectionPtr self = shared_from_this();

    _socket_ptr->async_write(_write_buffer, [self, this](errorcode const & ec, size_t bytes_transferred)
    {
        if (ec)
        {
            return;
        }

        _write_buffer.consume(bytes_transferred);

        if (_request.IsKeepAlive() && _response.IsKeepAlive())
        {
            this->reset();
            this->do_read();
        }
    });
}

void Connection::prepare_response_headers_for_chunked_write()
{
    if (!_response.IsChunked())
    {
        _response.MarkChunked();
    }

    auto v = _response.GetVersion();

    if (v.first < 1 || (v.first == 1 && v.second == 0))
    {
        _response.SetVersion(1, 1);
    }

    if (!_response.IsClose())
    {
        _response.MarkKeepAlive();
    }
}

void Connection::do_write_header_for_chunked()
{
    prepare_response_headers_for_chunked_write();

    std::ostream os(&_write_buffer);
    os << _response;

    ConnectionPtr self = shared_from_this();
    _socket_ptr->async_write(_write_buffer
                             , [self, this](errorcode const & ec, size_t bytes_transferred)
    {
        if (ec)
        {
            return;
        }

        _write_buffer.consume(bytes_transferred);
        do_write_next_chunked_body();
    });
}

void Connection::do_write_next_chunked_body(std::clock_t start_wait_clock)
{
    bool is_last = false;
    _current_chunked_body_buffer = _response.PopChunkedBody(is_last);

    if (is_last)
    {
        return;
    }

    ConnectionPtr self = shared_from_this();

    if (_current_chunked_body_buffer.empty())
    {
        std::clock_t now = std::clock();

        if (start_wait_clock > 0 && (now - start_wait_clock) / CLOCKS_PER_SEC > 5)
        {
            log::Server()->error("Wait next chunked response data timeout.");
            return;
        }

        if (start_wait_clock == 0)
        {
            start_wait_clock = now;
        }

        _socket_ptr->get_ioc().post(std::bind(&Connection::do_write_next_chunked_body
                                              , self, start_wait_clock));
    }
    else
    {
        _socket_ptr->async_write(_current_chunked_body_buffer
                                 , [self](errorcode const & ec, size_t bytes_transferred)
        {
            self->do_write_chunked_body_finished(ec, bytes_transferred);
        });
    }
}

void Connection::do_write_chunked_body_finished(errorcode const& ec, size_t bytes_transferred)
{
    if (ec)
    {
        log::Server()->warn("Write chunked body fail. body size {}, transferred {}. {}"
                            , _current_chunked_body_buffer.size(), bytes_transferred, ec.message());
        return;
    }

    do_write_next_chunked_body();
}

void Connection::reset()
{
    _url_buffer.clear();
    _body_buffer.clear();
    _reading_header_part = header_none_part;
    _reading_header_field.clear();
    _reading_header_value.clear();
    _read_complete = read_none_complete;
    _reading_part.Clear();
    _reading_part_buffer.clear();
    _multipart_parse_part = mp_parse_none;
    _request.Reset();
    _response.Reset();
    _app.reset();
}

} //namespace da4qi4
