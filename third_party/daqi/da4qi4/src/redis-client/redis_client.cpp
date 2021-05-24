#include "daqi/redis-client/redis_client.hpp"

#include <iostream>
#include <functional>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "daqi/def/log_def.hpp"

#include "daqi/redis-client/redis_command.hpp"
#include "daqi/redis-client/redis_parser.hpp"


namespace da4qi4
{

namespace
{

unsigned int redis_server_reconnect_interval_seconds(unsigned int const reconnect_count)
{
    if (reconnect_count < 10) //0s~10s
    {
        return 1;
    }

    if (reconnect_count < 32) //10s~2m
    {
        return 5;
    }

    //after 2 minutes
    if (reconnect_count < 200) //2m~0.5h
    {
        return 10;
    }

    if (reconnect_count < 260)  //0.5h~1h
    {
        return 30;
    }

    // 1h
    return 300; //reconnect 1 time per 5 minutes
}

} //namespace

RedisClient::~RedisClient()
{
    Disconnect();
}

void RedisClient::Connect(std::function<void (boost::system::error_code const& ec)> on,
                          std::string const& host, unsigned short port)
{
    if (IsConnected() || IsConnectting())
    {
        return;
    }

    _host = host;
    _port = port;

    do_async_connect(on);
}

void RedisClient::Reconnect(std::function<void (boost::system::error_code const& ec)> on)
{
    if (IsConnectting())
    {
        if (on)
        {
            auto ec = boost::system::errc::make_error_code(boost::system::errc::operation_in_progress);
            on(ec);
        }

        return;
    }

    if (!IsConnected())
    {
        boost::system::error_code ec;
        _reconnect_timer.cancel(ec);
        start_reconnect_timer(on);
    }
    else if (on)
    {
        auto ec = boost::system::errc::make_error_code(boost::system::errc::already_connected);
        on(ec);
    }
}

void RedisClient::do_async_connect(std::function<void (boost::system::error_code const& ec)> on)
{
    _connect_status = is_connectting;
    boost::asio::ip::tcp::endpoint end_point(
                boost::asio::ip::address::from_string(_host), _port);

    auto cb = std::bind(&RedisClient::on_connect_finished, this, on, std::placeholders::_1);
    _socket.async_connect(end_point, cb);
}

enum class AsyncHandlerAction
{
    on_async_connect, on_async_read, on_async_write, on_reply_parse
};

char const* get_async_handler_action_name(AsyncHandlerAction aha)
{
    static char const* names[] = {"connect", "read", "write", "parse"};

    switch (aha)
    {
        case AsyncHandlerAction::on_async_connect:
            return names[0];

        case AsyncHandlerAction::on_async_read:
            return names[1];

        case AsyncHandlerAction::on_async_write:
            return names[2];

        case AsyncHandlerAction::on_reply_parse:
            return names[3];
    }

    return "";
}

void try_on_redis_handler(std::function<void (boost::system::error_code const& ec)> on
                          , boost::system::error_code p, AsyncHandlerAction aha)
{
    if (!on)
    {
        return;
    }

    try
    {
        on(std::move(p));
    }
    catch (std::exception const& e)
    {
        log::Server()->error("Redis handle {} result exception. {}", get_async_handler_action_name(aha), e.what());
    }
    catch (std::string const& s)
    {
        log::Server()->error("Redis handle {} result exception. {}", get_async_handler_action_name(aha), s);
    }
    catch (char const* s)
    {
        log::Server()->error("Redis handle {} result exception. {}", get_async_handler_action_name(aha), s);
    }
    catch (...)
    {
        log::Server()->error("Redis handle {} result unknown exception.", get_async_handler_action_name(aha));
    }
}

void try_on_redis_handler(std::function<void(RedisValue value)> on, RedisValue p, AsyncHandlerAction aha)
{
    if (!on)
    {
        return;
    }

    //debug:
    RedisValue debug_dump;
    if (aha == AsyncHandlerAction::on_reply_parse)
    {
        debug_dump = p;
    }

    try
    {
        on(std::move(p));
    }
    catch (std::exception const& e)
    {
        log::Server()->error("Redis handle {} result exception. {}", get_async_handler_action_name(aha), e.what());

        if (!debug_dump.IsNull())
        {
           log::Server()->debug("context read from redis: {}.", debug_dump.ToString());
        }
    }
    catch (std::string const& s)
    {
        log::Server()->error("Redis handle {} result exception. {}", get_async_handler_action_name(aha), s);
    }
    catch (char const* s)
    {
        log::Server()->error("Redis handle {} result exception. {}", get_async_handler_action_name(aha), s);
    }
    catch (...)
    {
        log::Server()->error("Redis handle {} result unknown exception.", get_async_handler_action_name(aha));
    }
}

void RedisClient::on_connect_finished(std::function<void (boost::system::error_code const& ec)> on
                                      , boost::system::error_code const& ec)
{
    _connect_status = (!ec) ? is_connected : not_connect;

    if (ec)
    {
        _connect_status = not_connect;
        log::Server()->error("Connect to redis server fail. {}", ec.message());
    }
    else
    {
        if (_reconnect_count > 0)
        {
            log::Server()->info("Reconnect to redis server success, after {} time(s).", _reconnect_count);
            _reconnect_count = 0;
        }

        _connect_status = is_connected;
    }

    try_on_redis_handler(on, std::move(ec), AsyncHandlerAction::on_async_connect);

    if (ec && (_error_handle_policy == RedisClientErrorHandlePolicy::auto_reconnect))
    {
        start_reconnect_timer(on);
    }
}

bool RedisClient::start_reconnect_timer(std::function<void (boost::system::error_code const& ec)> on)
{
    auto seconds = redis_server_reconnect_interval_seconds(++_reconnect_count);

    boost::system::error_code timer_ec;
    _reconnect_timer.expires_from_now(boost::posix_time::seconds(seconds), timer_ec);

    if (timer_ec)
    {
        log::Server()->error("Set reconnect redis server timer expires time fail. {}", timer_ec.message());
        return false;
    }

    _reconnect_timer.async_wait([this, on](boost::system::error_code const & ec)
    {
        if (ec)
        {
            log::Server()->error("Start reconnect redis server timer fail. {}", ec.message());
            return;
        }

        do_async_connect(on);
    });

    return true;
}

void RedisClient::Disconnect()
{
    if (IsConnected())
    {
        do_disconnect();
    }
    else
    {
        boost::system::error_code ec;
        _reconnect_timer.cancel(ec);
    }
}

void RedisClient::do_disconnect()
{
    boost::system::error_code ec;
    _socket.cancel(ec);
    _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    _socket.close(ec);
    _connect_status = not_connect;
}

#ifdef _DEBUG_REDIS_NEED_SYNC_OPERATOR_
bool RedisClient::do_sync_connect()
{
    _connect_status = is_connectting;

    boost::system::error_code ec;

    boost::asio::ip::tcp::endpoint end_point(boost::asio::ip::address::from_string(_host), _port);

    _socket.connect(end_point, ec);

    if (ec)
    {
        _connect_status = not_connect;
        return false;
    }

    _connect_status = is_connected;
    return true;
}

RedisValue RedisClient::CommandSync(std::string cmd, std::deque<RedisBuffer> args)
{
    assert(!cmd.empty());

    args.emplace_front(std::move(cmd));
    auto send_data = MakeCommand(args);

    boost::system::error_code ec;
    boost::asio::write(_socket, boost::asio::buffer(send_data), ec);

    if (ec)
    {
        return RedisValue(ec.message(), RedisValue::ErrorTag());
    }

    RedisParser parser;

    std::size_t beg = 0;
    bool completed = false;

    std::string read_data;
    read_data.reserve(_read_buffer_size_ << 1);

    while (!completed)
    {
        char tmp_buf[_read_buffer_size_];
        std::size_t read_size = _socket.read_some(boost::asio::buffer(tmp_buf, _read_buffer_size_), ec);

        if (ec)
        {
            return RedisValue(ec.message(), RedisValue::ErrorTag());
        }

        if (read_size > 0)
        {
            read_data.append(tmp_buf, read_size);
            size_t end = read_data.size();

            while (!completed && (beg < end))
            {
                auto result = parser.Parse(read_data.c_str() + beg, end - beg);

                switch (result.second)
                {
                    case RedisParser::Completed:
                        completed = true;
                        break;

                    case RedisParser::Incompleted:
                        beg += result.first;
                        break;

                    default :
                        return RedisValue("parse fail", RedisValue::ErrorTag());
                }
            }
        }
    }

    return (completed) ? parser.Result() : RedisValue("parse fail", RedisValue::ErrorTag());
}

bool RedisClient::ConnectSync(std::string const& host, unsigned short port)
{
    if (IsConnected() || IsConnectting())
    {
        return true;
    }

    _host = host;
    _port = port;

    return do_sync_connect();
}
#endif //_DEBUG_REDIS_NEED_SYNC_OPERATOR_

void RedisClient::Command(std::string cmd, std::deque<RedisBuffer> args,
                          std::function<void(RedisValue value)> on)
{
    assert(!cmd.empty());

    args.emplace_front(std::move(cmd));
    auto command_data = MakeCommand(args);

    bool is_stopped_because_empty = _command_queue.empty();
    _command_queue.emplace(command_data, on);

    if (is_stopped_because_empty)
    {
        start_async_write();
    }
}

void RedisClient::start_async_write()
{
    if (_command_queue.empty())
    {
        return;
    }

    boost::asio::async_write(_socket, boost::asio::buffer(_command_queue.front().first)
                             , [this](boost::system::error_code const & ec,  std::size_t /*bytes*/)
    {
        assert(!_command_queue.empty());

        auto node(std::move(_command_queue.front()));
        auto on = node.second;

        if (ec)
        {
            RedisValue error_value(ec.message(), RedisValue::ErrorTag());
            try_on_redis_handler(on, std::move(error_value), AsyncHandlerAction::on_async_write);
            _command_queue.pop();

            if (_error_handle_policy == RedisClientErrorHandlePolicy::auto_reconnect)
            {
                Reconnect();
            }

            return;
        }

        start_aysnc_read_and_parse(on);
    });

}

void RedisClient::start_aysnc_read_and_parse(std::function<void(RedisValue value)> on)
{
    if (!_reply_buf.empty())
    {
        if (_reply_buf.size() <= _reply_parse_beg)
        {
            _reply_buf.clear();
        }
        else
        {
            _reply_buf = _reply_buf.substr(_reply_parse_beg);
        }
    }

    _reply_parse_beg = 0;

    std::shared_ptr<RedisParser> parser(new RedisParser);
    do_async_read_and_parse(parser, on);
}

void RedisClient::do_async_read_and_parse(std::shared_ptr<RedisParser> parser,
                                          std::function<void(RedisValue value)> on)
{
    _socket.async_read_some(boost::asio::buffer(_tmp_read_buf, _read_buffer_size_)
                            , [this, parser, on](boost::system::error_code const & ec, size_t size)
    {
        if (ec)
        {
            RedisValue error_value(ec.message(), RedisValue::ErrorTag());
            try_on_redis_handler(on, std::move(error_value), AsyncHandlerAction::on_async_read);
            _command_queue.pop();

            return;
        }

        if (size > 0)
        {
            _reply_buf.append(_tmp_read_buf, size);
            size_t end = _reply_buf.size();

            bool completed = false, error = false;

            while (!completed && !error && (_reply_parse_beg < end))
            {
                auto result = parser->Parse(_reply_buf.c_str() + _reply_parse_beg, end - _reply_parse_beg);
                _reply_parse_beg += result.first;

                switch (result.second)
                {
                    case RedisParser::Completed:
                        completed = true;
                        break;

                    case RedisParser::Incompleted:
                        break;

                    default :
                        error = true;
                        break;
                }
            }

            if (completed)
            {
                RedisValue value = parser->Result();
                try_on_redis_handler(on, std::move(value), AsyncHandlerAction::on_reply_parse);
                _command_queue.pop();
                start_async_write();
                return;
            }

            if (error)
            {
                RedisValue error_value("parse error. " + _reply_buf, RedisValue::ErrorTag());
                try_on_redis_handler(on, std::move(error_value), AsyncHandlerAction::on_reply_parse);
                _command_queue.pop();
                start_async_write();
                return;
            }
        } //read size (more than 0) byte.

        do_async_read_and_parse(parser, on);
    });
}

} // namespace da4qi4

