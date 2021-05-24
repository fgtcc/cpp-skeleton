#include "daqi/context.hpp"

#include <iterator>

#include "daqi/def/log_def.hpp"
#include "daqi/connection.hpp"
#include "daqi/application.hpp"


namespace da4qi4
{

std::string ContextIMP::session_data_name = "_session_data_";
std::string ContextIMP::model_data_name = "_model_data_";

RedisClientPtr init_redis_client(ConnectionPtr cnt)
{
    size_t index = cnt->GetIOContextIndex();
    return RedisPool().Get(index);
}

Context ContextIMP::Make(ConnectionPtr cnt)
{
    return std::shared_ptr<ContextIMP>(new ContextIMP(cnt));
}

ContextIMP::ContextIMP(ConnectionPtr cnt)
    : _cnt(cnt)
    , _env(cnt->HasApplication() ? cnt->GetApplication()->GetTemplateRoot().native() : "")
    , _redis(init_redis_client(cnt))
{
    prepair_template_env();
}

ContextIMP::~ContextIMP()
{
}

void ContextIMP::prepair_template_env()
{
    init_template_env(_env);

    if (_cnt->HasApplication())
    {
        _cnt->GetApplication()->GetTemplates().CopyIncludeTemplateTo(_env);
    }

    regist_template_enginer_common_functions();
}

size_t ContextIMP::IOContextIndex() const
{
    return _cnt->GetIOContextIndex();
}

log::LoggerPtr ContextIMP::logger()
{
    assert(_cnt);
    assert(_cnt->GetApplication());
    assert(_cnt->GetApplication()->GetLogger());

    return _cnt->GetApplication()->GetLogger();
}

void ContextIMP::regist_string_function_with_one_string_parameter(char const* function_name,
                                                                  PSSFun func,
                                                                  std::string defaultValue)
{
    _env.add_callback(function_name, 1
                      , [this, func, function_name, defaultValue](inja::Arguments & args) -> std::string
    {
        try
        {
            if (args.size() == 1)
            {
                std::string name = args.at(0)->get<std::string>();

                if (!name.empty())
                {
                    return (this->*func)(name);
                }
            }

            return defaultValue;
        }
        catch (std::exception const& e)
        {
            logger()
            ->error("Regist templage enginer function {} exception. {}", function_name, e.what());
        }

        return defaultValue;
    });
}

void ContextIMP::regist_bool_function_with_one_string_parameter(char const* function_name,
                                                                PBSFun func,
                                                                bool defaultValue)
{
    _env.add_callback(function_name, 1
                      , [this, func, function_name, defaultValue](inja::Arguments & args) -> bool
    {
        try
        {
            if (args.size() == 1)
            {
                std::string name = args.at(0)->get<std::string>();

                if (!name.empty())
                {
                    return (this->*func)(name);
                }
            }

            return defaultValue;
        }
        catch (std::exception const& e)
        {
            logger()
            ->error("Regist templage enginer function {} exception. {}"
                    , function_name, e.what());
        }

        return defaultValue;
    });
}

void ContextIMP::regist_template_enginer_common_functions()
{
    regist_string_function_with_one_string_parameter("_PARAMETER_", &Self::parameter);
    regist_bool_function_with_one_string_parameter("_IS_EXISTS_PARAMETER_", &Self::is_exists_parameter);

    regist_string_function_with_one_string_parameter("_HEADER_", &Self::header);
    regist_bool_function_with_one_string_parameter("_IS_EXISTS_HEADER_", &Self::is_exists_header);

    regist_string_function_with_one_string_parameter("_URL_PARAMETER_", &Self::url_parameter);
    regist_bool_function_with_one_string_parameter("_IS_EXISTS_URL_PARAMETER_", &Self::is_exists_url_parameter);

    regist_string_function_with_one_string_parameter("_PATH_PARAMETER_", &Self::path_parameter);
    regist_bool_function_with_one_string_parameter("_IS_EXISTS_PATH_PARAMETER_", &Self::is_exists_path_parameter);

    regist_string_function_with_one_string_parameter("_FORM_DATA_", &Self::form_data);
    regist_bool_function_with_one_string_parameter("_IS_EXISTS_FORM_DATA_", &Self::is_exists_form_data);

    regist_string_function_with_one_string_parameter("_COOKIE_", &Self::cookie);
    regist_bool_function_with_one_string_parameter("_IS_EXISTS_COOKIE_", &Self::is_exists_cookie);
}

Request const& ContextIMP::Req() const
{
    return _cnt->GetRequest();
}

Response& ContextIMP::Res()
{
    return _cnt->GetResponse();
}

Application&  ContextIMP::App()
{
    assert(_cnt->GetApplication() != nullptr);
    return *(_cnt->GetApplication());
}

Application const& ContextIMP::App() const
{
    assert(_cnt->GetApplication() != nullptr);
    return *(_cnt->GetApplication());
}

IOC& ContextIMP::IOContext()
{
#ifdef HAS_IO_CONTEXT
    return _cnt->GetSocket().get_executor().context();
#else
    return _cnt->GetSocket().get_io_service();
#endif
}

void ContextIMP::InitRequestPathParameters(std::vector<std::string> const& names
                                           , std::vector<std::string> const& values)
{
    _cnt->GetRequest().InitPathParameters(names, values);
}

void ContextIMP::InitRequestPathParameter(std::string const& value)
{
    this->InitRequestPathParameters({""}, {value});
}

std::string ContextIMP::GetSessionID() const
{
    Json const& session = this->SessionData();

    if (session.is_discarded() || session.is_null() || !session.is_object())
    {
        return Utilities::theEmptyString;
    }

    Cookie cookie = da4qi4::GetSessionCookie(session);

    if (cookie.IsEmpty())
    {
        return Utilities::theEmptyString;
    }

    return cookie.GetValue();
}

Cookie ContextIMP::GetSessionCookie() const
{
    Cookie cookie;

    Json const& session = this->SessionData();

    if (session.is_discarded() || session.is_null() || !session.is_object())
    {
        return cookie;
    }

    return da4qi4::GetSessionCookie(session);
}

void ContextIMP::ClearSessionData()
{
    auto& session = this->SessionData();

    for (auto it = session.begin(); it != session.end();)
    {
        if (it.key() != da4qi4::session_cookie_name)
        {
            it = session.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

std::string ContextIMP::render_on_template(std::string const& templ_name, Template const& templ
                                           , Json const& data
                                           , bool& server_render_error
                                           , std::string& error_what)
{
    server_render_error = false;
    error_what.clear();

    if (templ_name != _template_name)
    {
        _template_name = templ_name;
    }

    try
    {
        return _env.render(templ, data);
    }
    catch (std::runtime_error const& e)
    {
        server_render_error = true;
        error_what = e.what();
    }
    catch (std::exception const& e)
    {
        server_render_error = true;
        error_what = e.what();
    }
    catch (std::string const& s)
    {
        server_render_error = true;
        error_what = s;
    }
    catch (...)
    {
        server_render_error = true;
        error_what = "unknown render error.";
    }

    if (server_render_error)
    {
        logger()->error("Render template exception. {}. \"{}\".", error_what, templ_name);
    }

    return Utilities::theEmptyString;
}

void ContextIMP::render_on_template(std::string const& templ_name, Template const& templ
                                    , Json const& data, http_status status)
{
    bool error = false;
    std::string error_detail;

    std::string view = render_on_template(templ_name, templ, data, error, error_detail);

    if (error)
    {
        if (status != HTTP_STATUS_INTERNAL_SERVER_ERROR)
        {
            Json error_data;
            error_data["internal_server_error_detail"] = error_detail;
            RenderInternalServerError(error_data);
        }
        else
        {
            Res().ReplyStatus(status);
        }

        return;
    }

    Res().SetStatusCode(status);

    if (!view.empty())
    {
        auto content_type = Res().GetContentType(Response::ContentTypePart::without_chartset);

        if (content_type.empty())
        {
            Res().SetContentType("text/html");
        }

        Res().SetBody(std::move(view));
    }
}

ContextIMP& ContextIMP::Render()
{
    Json& page_data = ModelData();
    return (page_data.is_null()) ? this->RenderWithoutData() : this->render_with_data(page_data);
}

ContextIMP& ContextIMP::Render(Json const& data)
{
    return render_with_data(data);
}

ContextIMP& ContextIMP::Render(std::string const& template_name, Json const& data)
{
    if (!data.is_null())
    {
        return render_with_data(template_name, data);
    }

    Json& page_data = ModelData();
    return (page_data.is_null()) ?
           RenderWithoutData(template_name) : render_with_data(template_name, page_data);
}

ContextIMP& ContextIMP::Render(std::string const& template_name, http_status status, Json const& data)
{
    if (!data.is_null())
    {
        return render_with_data(template_name, status, data);
    }

    Json& page_data = ModelData();

    return (page_data.is_null()) ?
           RenderWithoutData(template_name, status) : render_with_data(template_name, status, page_data);
}

ContextIMP& ContextIMP::render_with_data(http_status status, Json const& data)
{
    std::string template_name = std::to_string(static_cast<int>(status));

    if (auto templ = App().GetTemplates().Get(template_name))
    {
        render_on_template(template_name, *templ, data, status);
    }
    else
    {
        Res().ReplyStatus(status);
    }

    return *this;
}

ContextIMP& ContextIMP::render_with_data(std::string const& template_name, Json const& data)
{
    if (template_name.empty())
    {
        return render_with_data(data);
    }

    auto templ = App().GetTemplates().Get(template_name);

    if (!templ)
    {
        logger()->error("No found template {0}.", template_name);
        RenderNofound();
        return *this;
    }

    render_on_template(template_name, *templ, data, HTTP_STATUS_OK);
    return *this;
}

std::string ContextIMP::auto_match_template()
{
    std::string const& path = Req().GetUrl().path;

    if (path.empty())
    {
        return "/index";
    }

    std::string::const_iterator first = path.begin();
    std::string::const_reverse_iterator last = path.crbegin();

    if (first == std::prev(last.base())) // path.size() == 1
    {
        return (*first == '/') ? "/index" : "";
    }

    if (*last != '/' && *first == '/')
    {
        return path;
    }

    std::string template_name;
    template_name.reserve(path.size() + 2);

    if (*first != '/')
    {
        template_name = "/";
    }

    template_name += path;

    if (*last == '/')
    {
        template_name += "index";
    }

    return template_name;
}

ContextIMP& ContextIMP::render_with_data(std::string const& template_name, http_status status, Json const& data)
{
    std::string template_name_use;

    if (template_name.empty())
    {
        template_name_use = std::to_string(static_cast<int>(status));
    }
    else
    {
        template_name_use = template_name;
    }

    if (auto templ = App().GetTemplates().Get(template_name))
    {
        render_on_template(template_name, *templ, data, status);
    }
    else
    {
        Res().ReplyStatus(status);
    }

    return *this;
}

ContextIMP& ContextIMP::render_with_data(Json const& data)
{
    if (!_template_name.empty()) // explicit template name
    {
        if (auto templ = App().GetTemplates().Get(_template_name))
        {
            render_on_template(_template_name, *templ, data, HTTP_STATUS_OK);
        }
        else
        {
            RenderNofound();
        }

        return *this;
    }

    std::string auto_matched_template_name = auto_match_template();

    if (auto_matched_template_name.empty())
    {
        RenderNofound();
        return *this;
    }

    auto templ = App().GetTemplates().Get(auto_matched_template_name);

    if (templ)
    {
        render_on_template(auto_matched_template_name, *templ, data, HTTP_STATUS_OK);
        return *this;
    }

    RenderNofound();
    return *this;
}

void ContextIMP::end()
{
    _cnt->StartWrite();
}

void ContextIMP::StartChunkedResponse()
{
    Res().MarkChunked();
}

void ContextIMP::NextChunkedResponse(std::string const& body)
{
    Res().PushChunkedBody(body, false);
}

void ContextIMP::StopChunkedResponse()
{
    Res().PushChunkedBody(Utilities::theEmptyString, true);
}

void ContextIMP::Start()
{
    _intercepter_on = Intercepter::On::Request;

    auto range = App().GetIntercepterChainRange();
    _intercepter_beg = _intercepter_iter = range.first;
    _intercepter_end = range.second;

    do_intercepter_on_req_dir();
}

void ContextIMP::Pass()
{
    next(Intercepter::Result::Pass);
}

void ContextIMP::Stop()
{
    next(Intercepter::Result::Stop);
}

void ContextIMP::do_intercepter_on_req_dir()
{
    assert(_intercepter_on == Intercepter::On::Request);

    if (_intercepter_iter == _intercepter_end)
    {
        _intercepter_on = Intercepter::On::Handle;
        App().Handle(shared_from_this());
        return;
    }

    auto& handler = *_intercepter_iter; //ref!!
    handler(shared_from_this(), Intercepter::On::Request);
}

void ContextIMP::do_intercepter_on_res_dir()
{
    assert(_intercepter_on == Intercepter::On::Response);

    Intercepter::ChainReverseIterator r_intercepter_iter(_intercepter_iter);
    Intercepter::ChainReverseIterator r_end(_intercepter_beg);

    if (r_intercepter_iter == r_end)
    {
        end();
        return;
    }

    auto& handler = *r_intercepter_iter; //ref!!
    handler(shared_from_this(), Intercepter::On::Response);
}

void ContextIMP::next(Intercepter::Result result)
{
    if (_intercepter_on == Intercepter::On::Request)
    {
        next_intercepter_on_req_dir(result);
    }
    else if (_intercepter_on == Intercepter::On::Handle)
    {
        _intercepter_on = Intercepter::On::Response;
        start_intercepter_on_res_dir(result);
    }
    else if (_intercepter_on == Intercepter::On::Response)
    {
        next_intercepter_on_res_dir(result);
    }
}

void ContextIMP::next_intercepter_on_req_dir(Intercepter::Result result)
{
    assert(_intercepter_on == Intercepter::On::Request);

    ++_intercepter_iter;

    switch (result)
    {
        case Intercepter::Result::Pass :
            do_intercepter_on_req_dir();
            break;

        case  Intercepter::Result::Stop :
            _intercepter_on = Intercepter::On::Response;
            do_intercepter_on_res_dir();
            break;
    }
}

void ContextIMP::start_intercepter_on_res_dir(Intercepter::Result result)
{
    switch (result)
    {
        case Intercepter::Result::Pass :
            do_intercepter_on_res_dir();
            break;

        case  Intercepter::Result::Stop :
            end();
            break;
    }
}

void ContextIMP::next_intercepter_on_res_dir(Intercepter::Result result)
{
    assert(_intercepter_on == Intercepter::On::Response);

    --_intercepter_iter;
    start_intercepter_on_res_dir(result);
}

} //namespace da4qi4
