#ifndef RS_TYPEDEFS_HPP
#define RS_TYPEDEFS_HPP

#include <restinio/all.hpp>
#include <restinio/router/easy_parser_router.hpp>
#include <nlohmann/json.hpp>
#include <soci/soci.h>
#include <boost/lexical_cast.hpp>// for lexical_cast() 
#include "3rd_party/magic_enum.hpp"

/* Fix cast true -> 1 and false -> 0 */
namespace boost {
    template<> 
    bool lexical_cast<bool, std::string>(const std::string& arg) {
        std::istringstream ss(arg);
        bool b;
        ss >> std::boolalpha >> b;
        return b;
    }

    template<>
    std::string lexical_cast<std::string, bool>(const bool& b) {
        std::ostringstream ss;
        ss << std::boolalpha << b;
        return ss.str();
    }
}

namespace rs {
struct unit {};

using json_t = nlohmann::json;

namespace epr = restinio::router::easy_parser_router;
using router_t = restinio::router::easy_parser_router_t;
//using handler_t = restinio::router::express_request_handler_t;
using api_response_t = json_t;

template <class JParams, typename...RouteParams>
using api_handler_t = std::function<api_response_t(JParams, RouteParams...)>;

json_t success_response(std::string_view info = "") {
    json_t json;
    json["message"] = "OK";
    if (!info.empty())
        json["info"] = info;
    return json;
};

enum class ApiErrorId { 
    Other,
    InvalidParams,
    JsonParseError,
    NotFound,
    DBError 
};

constexpr inline const char * msgFromApiErrorId(ApiErrorId id) {
    switch(id) {
        case ApiErrorId::InvalidParams : return "Invalid parameters";
        case ApiErrorId::JsonParseError : return "Error while parsing JSON request";
        case ApiErrorId::DBError : return "Error with db query";
        case ApiErrorId::NotFound : return "Api Request not Found";
        default : return "Other error";
    }
}

inline restinio::http_status_line_t statusCodeFromApiErrorId(ApiErrorId id) {
    switch(id) {
        case ApiErrorId::InvalidParams : return restinio::status_bad_request();
        case ApiErrorId::JsonParseError : return restinio::status_bad_request();
        case ApiErrorId::DBError : return restinio::status_internal_server_error();
        case ApiErrorId::NotFound : return restinio::status_not_found();
        default : return restinio::status_internal_server_error();
    }
}

class ApiError {
    ApiErrorId m_id;
    json_t m_info;
public:
    ApiErrorId id() const { return m_id; }
    ApiError(ApiErrorId id, json_t && info = nullptr) : m_id(id), m_info(std::move(info)) {}
    json_t json() const {
        json_t j;
        j["error_id"] = magic_enum::enum_name(m_id);
        j["message"] = msgFromApiErrorId(m_id);

        if (!m_info.is_null())
            j["info"] = m_info;

        return j;
    }
    const char * msg() const {
        return msgFromApiErrorId(m_id); 
    }

    restinio::http_status_line_t status() const {
        return statusCodeFromApiErrorId(m_id); 
    }
};

class ApiException : public ApiError, public std::exception {
 public:
    ApiException(ApiErrorId id, json_t && info = { nullptr }) : ApiError(id, std::move(info)) {}
    const char * what() const noexcept override { return this->msg(); }
};

}

#endif
