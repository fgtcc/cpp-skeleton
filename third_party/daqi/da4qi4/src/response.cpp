#include "daqi/response.hpp"

#include <cstring>
#include <map>
#include <sstream>

#include "daqi/utilities/string_utilities.hpp"
#include "daqi/utilities/container_utilities.hpp"
#include "daqi/utilities/html_utilities.hpp"

namespace da4qi4
{

std::string const& EmptyBody()
{
    return Utilities::theEmptyString;
}

struct HttpStatusSummaryHelper
{
    friend char const* HttpStatusSummaryGetter(int);
private:
    HttpStatusSummaryHelper()
    {
        init();
    }

    void init();
    std::map<int, char const*> _map;
};

#define ADD_SUMMARY(CODE, SUMMARY) _map[CODE] = #SUMMARY

void HttpStatusSummaryHelper::init()
{
    ADD_SUMMARY(HTTP_STATUS_CONTINUE,                        Continue)                        ;
    ADD_SUMMARY(HTTP_STATUS_SWITCHING_PROTOCOLS,             Switching Protocols)             ;
    ADD_SUMMARY(HTTP_STATUS_PROCESSING,                      Processing)                      ;
    ADD_SUMMARY(HTTP_STATUS_OK,                              OK)                              ;
    ADD_SUMMARY(HTTP_STATUS_CREATED,                         Created)                         ;
    ADD_SUMMARY(HTTP_STATUS_ACCEPTED,                        Accepted)                        ;
    ADD_SUMMARY(HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION,   Non - Authoritative Information)   ;
    ADD_SUMMARY(HTTP_STATUS_NO_CONTENT,                      No Content)                      ;
    ADD_SUMMARY(HTTP_STATUS_RESET_CONTENT,                   Reset Content)                   ;
    ADD_SUMMARY(HTTP_STATUS_PARTIAL_CONTENT,                 Partial Content)                 ;
    ADD_SUMMARY(HTTP_STATUS_MULTI_STATUS,                    Multi - Status)                    ;
    ADD_SUMMARY(HTTP_STATUS_ALREADY_REPORTED,                Already Reported)                ;
    ADD_SUMMARY(HTTP_STATUS_IM_USED,                         IM Used)                         ;
    ADD_SUMMARY(HTTP_STATUS_MULTIPLE_CHOICES,                Multiple Choices)                ;
    ADD_SUMMARY(HTTP_STATUS_MOVED_PERMANENTLY,               Moved Permanently)               ;
    ADD_SUMMARY(HTTP_STATUS_FOUND,                           Found)                           ;
    ADD_SUMMARY(HTTP_STATUS_SEE_OTHER,                       See Other)                       ;
    ADD_SUMMARY(HTTP_STATUS_NOT_MODIFIED,                    Not Modified)                    ;
    ADD_SUMMARY(HTTP_STATUS_USE_PROXY,                       Use Proxy)                       ;
    ADD_SUMMARY(HTTP_STATUS_TEMPORARY_REDIRECT,              Temporary Redirect)              ;
    ADD_SUMMARY(HTTP_STATUS_PERMANENT_REDIRECT,              Permanent Redirect)              ;
    ADD_SUMMARY(HTTP_STATUS_BAD_REQUEST,                     Bad Request)                     ;
    ADD_SUMMARY(HTTP_STATUS_UNAUTHORIZED,                    Unauthorized)                    ;
    ADD_SUMMARY(HTTP_STATUS_PAYMENT_REQUIRED,                Payment Required)                ;
    ADD_SUMMARY(HTTP_STATUS_FORBIDDEN,                       Forbidden)                       ;
    ADD_SUMMARY(HTTP_STATUS_NOT_FOUND,                       Not Found)                       ;
    ADD_SUMMARY(HTTP_STATUS_METHOD_NOT_ALLOWED,              Method Not Allowed)              ;
    ADD_SUMMARY(HTTP_STATUS_NOT_ACCEPTABLE,                  Not Acceptable)                  ;
    ADD_SUMMARY(HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED,   Proxy Authentication Required)   ;
    ADD_SUMMARY(HTTP_STATUS_REQUEST_TIMEOUT,                 Request Timeout)                 ;
    ADD_SUMMARY(HTTP_STATUS_CONFLICT,                        Conflict)                        ;
    ADD_SUMMARY(HTTP_STATUS_GONE,                            Gone)                            ;
    ADD_SUMMARY(HTTP_STATUS_LENGTH_REQUIRED,                 Length Required)                 ;
    ADD_SUMMARY(HTTP_STATUS_PRECONDITION_FAILED,             Precondition Failed)             ;
    ADD_SUMMARY(HTTP_STATUS_PAYLOAD_TOO_LARGE,               Payload Too Large)               ;
    ADD_SUMMARY(HTTP_STATUS_URI_TOO_LONG,                    URI Too Long)                    ;
    ADD_SUMMARY(HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE,          Unsupported Media Type)          ;
    ADD_SUMMARY(HTTP_STATUS_RANGE_NOT_SATISFIABLE,           Range Not Satisfiable)           ;
    ADD_SUMMARY(HTTP_STATUS_EXPECTATION_FAILED,              Expectation Failed)              ;
    ADD_SUMMARY(HTTP_STATUS_MISDIRECTED_REQUEST,             Misdirected Request)             ;
    ADD_SUMMARY(HTTP_STATUS_UNPROCESSABLE_ENTITY,            Unprocessable Entity)            ;
    ADD_SUMMARY(HTTP_STATUS_LOCKED,                          Locked)                          ;
    ADD_SUMMARY(HTTP_STATUS_FAILED_DEPENDENCY,               Failed Dependency)               ;
    ADD_SUMMARY(HTTP_STATUS_UPGRADE_REQUIRED,                Upgrade Required)                ;
    ADD_SUMMARY(HTTP_STATUS_PRECONDITION_REQUIRED,           Precondition Required)           ;
    ADD_SUMMARY(HTTP_STATUS_TOO_MANY_REQUESTS,               Too Many Requests)               ;
    ADD_SUMMARY(HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) ;
    ADD_SUMMARY(HTTP_STATUS_UNAVAILABLE_FOR_LEGAL_REASONS,   Unavailable For Legal Reasons)   ;
    ADD_SUMMARY(HTTP_STATUS_INTERNAL_SERVER_ERROR,           Internal Server Error)           ;
    ADD_SUMMARY(HTTP_STATUS_NOT_IMPLEMENTED,                 Not Implemented)                 ;
    ADD_SUMMARY(HTTP_STATUS_BAD_GATEWAY,                     Bad Gateway)                     ;
    ADD_SUMMARY(HTTP_STATUS_SERVICE_UNAVAILABLE,             Service Unavailable)             ;
    ADD_SUMMARY(HTTP_STATUS_GATEWAY_TIMEOUT,                 Gateway Timeout)                 ;
    ADD_SUMMARY(HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED,      HTTP Version Not Supported)      ;
    ADD_SUMMARY(HTTP_STATUS_VARIANT_ALSO_NEGOTIATES,         Variant Also Negotiates)         ;
    ADD_SUMMARY(HTTP_STATUS_INSUFFICIENT_STORAGE,            Insufficient Storage)            ;
    ADD_SUMMARY(HTTP_STATUS_LOOP_DETECTED,                   Loop Detected)                   ;
    ADD_SUMMARY(HTTP_STATUS_NOT_EXTENDED,                    Not Extended)                    ;
    ADD_SUMMARY(HTTP_STATUS_NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required) ;
}

char const* HttpStatusSummaryGetter(int code)
{
    static HttpStatusSummaryHelper rsm;
    std::map<int, char const*>::const_iterator cit  = rsm._map.find(code);
    return (cit == rsm._map.cend()) ? "" : cit->second;
}

void ChunkedBodies::PushBack(std::string const& body, bool is_last)
{
    std::string data;
    data.reserve(body.size() + 5);

    if (is_last)
    {
        data = "0\r\n\r\n";
    }
    else
    {
        std::stringstream ss;
        ss << Utilities::DecIntToHexStr(body.size()) << "\r\n";
        ss << body << "\r\n";
        data = ss.str();
    }

    std::lock_guard<std::mutex> lock(_m);
    _chunked_bodies.push_back(std::make_pair(std::move(data), false));

    if (is_last)
    {
        _chunked_bodies.push_back(std::make_pair("", true));
    }
}

std::string ChunkedBodies::PopFront(bool& is_last)
{
    is_last = false;
    std::lock_guard<std::mutex> lock(_m);

    if (_chunked_bodies.empty())
    {
        return Utilities::theEmptyString;
    }

    std::pair<std::string, bool> front(std::move(*_chunked_bodies.begin()));
    _chunked_bodies.erase(_chunked_bodies.begin());

    is_last = front.second;
    return front.first;
}

void ChunkedBodies::Clear()
{
    std::lock_guard<std::mutex> lock(_m);
    _chunked_bodies.clear();
}

Response::Response()
{
    add_server_header();
}

void Response::add_server_header()
{
    std::stringstream ss;
    ss << the_daqi_name << '/' << the_daqi_version;
    _headers["Server"] = ss.str();
}

void Response::Reset()
{
    _status_code = HTTP_STATUS_OK;
    _headers.clear();
    _body.clear();
    _charset = "UTF-8";
    _version_major  = _version_minor = 1;
    _cookies.clear();
    _chunked_bodies.Clear();

    add_server_header();
}

void Response::AppendHeader(std::string const& field, std::string const& value)
{
    auto it = _headers.find(field);

    if (it != _headers.end())
    {
        it->second = std::move(value);
    }
    else
    {
        _headers.insert(std::make_pair(std::move(field), std::move(value)));
    }
}
bool Response::IsExistsHeader(std::string const& field) const
{
    return Utilities::IsExistsHeader(_headers, field);
}
std::string const& Response::GetHeader(std::string const& field) const
{
    return Utilities::GetHeader(_headers, field);
}
OptionalStringRefConst Response::TryGetHeader(std::string const& field) const
{
    return Utilities::TryGetHeader(_headers, field);
}
std::pair<std::string, std::string> split_content_type_value(std::string const& value)
{
    auto pair = std::make_pair(value, std::string());

    if (!value.empty())
    {
        auto pos = value.find(" ;");

        if (pos != std::string::npos)
        {
            pair.first = value.substr(0, pos);
            pair.second = value.substr(pos + 2);
        }
    }

    return pair;
}
std::string Response::GetContentType(ContentTypePart part) const
{
    std::string value = GetHeader("Content-Type");

    if (part == with_chartset)
    {
        return value;
    }

    if (!value.empty())
    {
        auto pos = value.find(";");

        if (pos != std::string::npos)
        {
            return value.substr(0, pos);
        }
    }

    return value;
}
bool is_content_type_need_charset(std::string const& content_type)
{
    return Utilities::iStartsWith(content_type, "text/");
}
void Response::SetCharset(std::string const& charset)
{
    _charset = charset;
    auto it = _headers.find("Content-Type");

    if (it != _headers.end())
    {
        auto pair = split_content_type_value(it->second);

        if (charset != pair.second
            && is_content_type_need_charset(pair.first))
        {
            SetContentType(pair.first, charset);
        }
    }
}
void Response::SetContentType(std::string const& content_type)
{
    std::string field_value;
    field_value.reserve(content_type.size() + _charset.size() + 2);
    field_value = content_type;

    if (is_content_type_need_charset(content_type) && !_charset.empty())
    {
        field_value += ("; charset=" + _charset);
    }

    AppendHeader("Content-Type", field_value);
}
void Response::SetContentType(std::string const& content_type
                              , std::string const& content_charset)
{
    std::string field_value;
    field_value = content_type + "; charset=" + content_charset;
    AppendHeader("Content-Type", field_value);
}
bool Response::IsChunked() const
{
    auto item = TryGetHeader("Transfer-Encoding");
    return (item && Utilities::iEquals(*item, "chunked"));
}
void Response::MarkKeepAlive()
{
    AppendHeader("Connection", "keep-alive");
}
void Response::MarkClose()
{
    AppendHeader("Connection", "close");
}
bool Response::IsKeepAlive() const
{
    auto value = TryGetHeader("Connection");

    if ((_version_major < 1) || (_version_major == 1 && _version_minor == 0))
    {
        return (value && Utilities::iEquals(*value, "keep-alive"));
    }
    else
    {
        return (!value || !Utilities::iEquals(*value, "close"));
    }
}
bool Response::IsClose() const
{
    return !IsKeepAlive();
}

void Response::set_or_default_body(std::string body, bool provide_default_if_body_is_empty)
{
    if (!body.empty())
    {
        SetBody(std::move(body));
        return;
    }

    if (provide_default_if_body_is_empty)
    {
        SetContentType("text/plain");
        SetBody(HttpStatusSummaryGetter(_status_code));
    }
}
void Response::ReplyStatus(int code, std::string body)
{
    _status_code = code;
    set_or_default_body(std::move(body));
}

void Response::ReplyOk()
{
    return ReplyOk(nullptr);
}

void Response::ReplyOk(const char* body)
{
    _status_code = HTTP_STATUS_OK;
    std::string s = (body ? std::string(body) : EmptyBody());
    set_or_default_body(std::move(s));
}

void Response::ReplyOk(std::string body)
{
    _status_code = HTTP_STATUS_OK;
    set_or_default_body(std::move(body));
}

void Response::ReplyOk(Json const& result)
{
    try
    {
        std::string body = result.dump(2);
        ReplyOkJSON(body);
    }
    catch (...)
    {
        ReplyInternalServerError();
    }
}


void Response::ReplyNofound(std::string body)
{
    _status_code = HTTP_STATUS_NOT_FOUND;
    set_or_default_body(std::move(body));
}

void Response::ReplyGone(std::string body)
{
    _status_code = HTTP_STATUS_GONE;
    set_or_default_body(std::move(body));
}

void Response::ReplyUnauthorized(std::string body)
{
    _status_code = HTTP_STATUS_UNAUTHORIZED;
    set_or_default_body(std::move(body));
}

void Response::ReplyNoAuthoritativeInfo(std::string body)
{
    _status_code = HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION;
    set_or_default_body(std::move(body));
}

void Response::ReplyBadRequest(std::string body)
{
    _status_code = HTTP_STATUS_BAD_REQUEST;
    set_or_default_body(std::move(body));
}

void Response::ReplyRangeNotSatisfiable(std::string body)
{
    _status_code = HTTP_STATUS_RANGE_NOT_SATISFIABLE;
    set_or_default_body(std::move(body));
}

void Response::ReplyForbidden(std::string body)
{
    _status_code = HTTP_STATUS_FORBIDDEN;
    set_or_default_body(std::move(body));
}

void Response::ReplyMethodNotAllowed(std::string body)
{
    _status_code = HTTP_STATUS_METHOD_NOT_ALLOWED;
    set_or_default_body(std::move(body));
}

void Response::ReplyHttpVersionNotSupported(std::string body)
{
    _status_code = HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED;
    set_or_default_body(std::move(body));
}

void Response::ReplyPayloadTooLarge(std::string body)
{
    _status_code = HTTP_STATUS_PAYLOAD_TOO_LARGE;
    set_or_default_body(std::move(body));
}

void Response::ReplyUriTooLong(std::string body)
{
    _status_code = HTTP_STATUS_URI_TOO_LONG;
    set_or_default_body(std::move(body));
}

void Response::ReplyTooManyRequests(std::string body)
{
    _status_code = HTTP_STATUS_TOO_MANY_REQUESTS;
    set_or_default_body(std::move(body));
}

void Response::ReplyNotImplemented(std::string body)
{
    _status_code = HTTP_STATUS_NOT_IMPLEMENTED;
    set_or_default_body(std::move(body));
}

void Response::ReplyUnsupportedMediaType(std::string body)
{
    _status_code = HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE;
    set_or_default_body(std::move(body));
}

void Response::ReplyServiceUnavailable(std::string body)
{
    _status_code = HTTP_STATUS_SERVICE_UNAVAILABLE;
    set_or_default_body(std::move(body));
}

void Response::ReplyInternalServerError(std::string body)
{
    _status_code = HTTP_STATUS_INTERNAL_SERVER_ERROR;
    set_or_default_body(std::move(body));
}

void Response::ReplyMovedPermanently(std::string const& dst_location, std::string body)
{
    _status_code = HTTP_STATUS_MOVED_PERMANENTLY;
    SetLocation(dst_location);
    set_or_default_body(std::move(body));
}

void Response::ReplyRedirect(std::string const& dst_location
                             , RedirectType type
                             , std::string body)
{
    _status_code = (type == RedirectType::permanent)
                   ? HTTP_STATUS_PERMANENT_REDIRECT
                   : HTTP_STATUS_TEMPORARY_REDIRECT;
    SetLocation(dst_location);
    set_or_default_body(std::move(body), false);
}

void Response::SetCookie(Cookie const& cookie)
{
    for (auto& c : _cookies)
    {
        if (Utilities::iEquals(c.GetName(), cookie.GetName())
            && Utilities::iEquals(c.GetDomain(), cookie.GetDomain())
            && Utilities::iEquals(c.GetPath(), cookie.GetPath()))
        {
            c = cookie;
            return;
        }
    }

    _cookies.push_back(cookie);
}

void Response::SetCookieExpiredImmediately(std::string const& name,
                                           std::string const& domain, std::string const& path)
{
    bool found = false;

    for (auto& c : _cookies)
    {
        if (Utilities::iEquals(c.GetName(), name)
            && Utilities::iEquals(c.GetDomain(), domain)
            && Utilities::iEquals(c.GetPath(), path))
        {
            c.SetValue("").SetExpiredImmediately();
            found = true;
            break;
        }
    }

    if (!found)
    {
        Cookie cookie(name, "", domain, path);
        cookie.SetExpiredImmediately();
        _cookies.push_back(cookie);
    }
}

std::ostream& operator << (std::ostream& os, Response const& res)
{
    int code = res.GetStatusCode();
    char const* summary = HttpStatusSummaryGetter(code);
    auto v = res.GetVersion();
    os << "HTTP/" << v.first << "." << v.second << ' ' << code << ' ' << summary << "\r\n";

    for (auto const& h : res.GetHeaders())
    {
        os << h.first << ":" << h.second << "\r\n";
    }

    for (auto const& cookie : res.GetCookies())
    {
        os << "Set-Cookie:" << cookie << "\r\n";
    }

    if (!res.IsChunked())
    {
        os << "Content-Length:" << res.GetBody().size() << "\r\n\r\n";
        os << res.GetBody();
    }
    else
    {
        os << "\r\n";
    }

    return os;
}

} //namespace da4qi4
