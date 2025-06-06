#ifndef CHARGELAB_OPEN_FIRMWARE_URI_H
#define CHARGELAB_OPEN_FIRMWARE_URI_H

#include "openocpp/helpers/string.h"
#include "openocpp/helpers/json.h"
#include "openocpp/helpers/optional.h"

#include <cstdlib>
#include <regex>

namespace chargelab::uri {
    namespace detail {
        inline std::pair<std::string, std::optional<std::string>> SplitLeft(std::string const& text, std::string const& delimiter) {
            auto it = text.find_first_of(delimiter);
            if (it == std::string::npos)
                return std::make_pair(text, std::nullopt);

            return std::make_pair(
                    text.substr(0, it),
                    std::make_optional(text.substr(it + delimiter.size()))
            );
        }

        inline std::pair<std::optional<std::string>, std::string> SplitRight(std::string const& text, std::string const& delimiter) {
            auto result = SplitLeft(text, delimiter);
            if (result.second.has_value()) {
                return std::make_pair(std::make_optional(result.first), result.second.value());
            } else {
                return std::make_pair(std::nullopt, result.first);
            }
        }

        int GetHexValue(char ch) {
            if (ch >= '0' && ch <= '9')
                return ch-'0';
            if (ch >= 'a' && ch <= 'f')
                return (ch-'a')+10;
            if (ch >= 'A' && ch <= 'F')
                return (ch-'A')+10;

            return -1;
        }
    }

    // TODO: Update to ws/wss
    CHARGELAB_JSON_ENUM(WebsocketScheme, 
        WS,
        WSS,
    )

    struct WebsocketParts {
        WebsocketScheme scheme = WebsocketScheme::kValueNotFoundInEnum;
        std::string host {};
        int port = -1;
        std::string path {};
        CHARGELAB_JSON_INTRUSIVE(WebsocketParts, scheme, host, port, path)
    };

    inline std::optional<WebsocketParts> ParseWebsocketUri(std::string const& uri) {
        WebsocketParts result {};

        auto scheme_fragment = detail::SplitLeft(uri, "://");
        if (!scheme_fragment.second.has_value())
            return std::nullopt;

        auto scheme = scheme_fragment.first;
        if (string::EqualsIgnoreCaseAscii(scheme, "ws")) {
            result.scheme = WebsocketScheme::kWS;
        } else if (string::EqualsIgnoreCaseAscii(scheme, "wss")) {
            result.scheme = WebsocketScheme::kWSS;
        } else {
            CHARGELAB_LOG_MESSAGE(error) << "Bad scheme in: " << uri;
            return std::nullopt;
        }

        auto target_fragment = detail::SplitLeft(scheme_fragment.second.value(), "/");
        auto host_port_fragment = detail::SplitLeft(target_fragment.first, ":");
        if (host_port_fragment.second.has_value()) {
            auto port = string::ToInteger(host_port_fragment.second.value());
            if (!port.has_value()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed parsing port text in: " << uri;
                return std::nullopt;
            }

            result.port = port.value();
        } else {
            switch (result.scheme) {
                default:
                    assert(false && "Unexpected websocket protocol");

                case WebsocketScheme::kWS:
                    result.port = 80;
                    break;

                case WebsocketScheme::kWSS:
                    result.port = 443;
                    break;
            }
        }

        result.host = host_port_fragment.first;
        result.path = "/" + optional::GetOrDefault<std::string>(target_fragment.second, "");
        CHARGELAB_LOG_MESSAGE(trace) << "Parsed websocket URI: " << result;

        return result;
    }

    CHARGELAB_JSON_ENUM(FtpScheme, 
        FTP,
        FTPS,
    )

    struct FtpParts {
        FtpScheme scheme = FtpScheme::kValueNotFoundInEnum;
        std::optional<std::string> username = std::nullopt;
        std::optional<std::string> password = std::nullopt;
        std::string host {};
        int port = -1;
        std::string path {};
        CHARGELAB_JSON_INTRUSIVE(FtpParts, scheme, username, password, host, port, path)
    };

    inline std::optional<FtpParts> ParseFtpUri(std::string const& uri) {
        FtpParts result {};

        auto scheme_fragment = detail::SplitLeft(uri, "://");
        if (!scheme_fragment.second.has_value())
            return std::nullopt;

        auto scheme = scheme_fragment.first;
        if (string::EqualsIgnoreCaseAscii(scheme, "ftp")) {
            result.scheme = FtpScheme::kFTP;
        } else if (string::EqualsIgnoreCaseAscii(scheme, "ftps")) {
            result.scheme = FtpScheme::kFTPS;
        } else {
            CHARGELAB_LOG_MESSAGE(error) << "Bad scheme in: " << uri;
            return std::nullopt;
        }

        auto target_fragment = detail::SplitLeft(scheme_fragment.second.value(), "/");
        auto credentials_fragment = detail::SplitRight(target_fragment.first, "@");
        auto host_port_fragment = detail::SplitLeft(credentials_fragment.second, ":");
        if (host_port_fragment.second.has_value()) {
            auto port = string::ToInteger(host_port_fragment.second.value());
            if (!port.has_value()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed parsing port text in: " << uri;
                return std::nullopt;
            }

            result.port = port.value();
        } else {
            result.port = 21;
        }

        result.host = host_port_fragment.first;
        result.path = "/" + optional::GetOrDefault<std::string>(target_fragment.second, "");

        if (credentials_fragment.first.has_value()) {
            auto username_password_fragment = detail::SplitLeft(credentials_fragment.first.value(), ":");
            result.username = username_password_fragment.first;
            if (username_password_fragment.second.has_value())
                result.password = username_password_fragment.second.value();
        }

        CHARGELAB_LOG_MESSAGE(trace) << "Parsed FTP URI: " << result;
        return result;
    }

    // TODO: Update to http/https
    CHARGELAB_JSON_ENUM(HttpScheme, 
        Http,
        Https,
    )

    struct HttpParts {
        HttpScheme scheme = HttpScheme::kValueNotFoundInEnum;
        std::string host {};
        int port = -1;
        std::string path {};
    };

    inline std::optional<HttpParts> parseHttpUri(std::string const& uri) {
        HttpParts result {};

        auto scheme_fragment = detail::SplitLeft(uri, "://");
        if (!scheme_fragment.second.has_value())
            return std::nullopt;

        auto scheme = scheme_fragment.first;
        if (string::EqualsIgnoreCaseAscii(scheme, "http")) {
            result.scheme = HttpScheme::kHttp;
        } else if (string::EqualsIgnoreCaseAscii(scheme, "https")) {
            result.scheme = HttpScheme::kHttps;
        } else {
            CHARGELAB_LOG_MESSAGE(error) << "Bad scheme in: " << uri;
            return std::nullopt;
        }

        auto target_fragment = detail::SplitLeft(scheme_fragment.second.value(), "/");
        auto host_port_fragment = detail::SplitLeft(target_fragment.first, ":");
        if (host_port_fragment.second.has_value()) {
            auto port = string::ToInteger(host_port_fragment.second.value());
            if (!port.has_value()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed parsing port text in: " << uri;
                return std::nullopt;
            }

            result.port = port.value();
        } else {
            switch (result.scheme) {
                default:
                    assert(false && "Unexpected websocket protocol");

                case HttpScheme::kHttp:
                    result.port = 80;
                    break;

                case HttpScheme::kHttps:
                    result.port = 443;
                    break;
            }
        }

        result.host = host_port_fragment.first;
        result.path = "/" + optional::GetOrDefault<std::string>(target_fragment.second, "");

        return result;
    }

    std::string decodeUriComponent(std::string const& text) {
        std::string result;
        for (std::size_t i=0; i < text.size(); i++) {
            // Note: allowing for deviations here; % prefixes not representing a valid octet and ignored
            if (text[i] == '%' && i+2 < text.size()) {
                auto const ch1 = detail::GetHexValue(text[i+1]);
                auto const ch2 = detail::GetHexValue(text[i+2]);
                if (ch1 >= 0 && ch2 >= 0) {
                    result += (char)((ch1 << 8) | ch2);
                    i += 2;
                    continue;
                }
            }

            result += text[i];
        }

        return result;
    }
}

#endif //CHARGELAB_OPEN_FIRMWARE_URI_H
