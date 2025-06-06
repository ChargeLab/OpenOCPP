#ifndef CHARGELAB_OPEN_FIRMWARE_PORTAL_DEMO_H
#define CHARGELAB_OPEN_FIRMWARE_PORTAL_DEMO_H

#include "openocpp/module/reset_module.h"
#include "openocpp/module/common_templates.h"
#include "openocpp/common/settings.h"
#include "openocpp/implementation/platform_esp.h"
#include "openocpp/implementation/station_test_esp32.h"
#include "openocpp/module/connector_status_module.h"
#include "openocpp/module/configuration_module.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_eth.h"

#include "esp_https_server.h"
#include "esp_tls.h"
#include "sdkconfig.h"

#include "esp_http_client.h"
#include "openocpp/protocol/ocpp1_6/messages/get_configuration.h"

namespace chargelab {
    namespace detail {
        struct DeviceConfigInfo {
            std::string centralSystemUrl;
            std::string chargePointId;
            std::string ocppProtocol; // OCPP16 or OCPP20
            int securityProfile;
            bool clearCertificates;
            std::string serialNumber;
            std::string maxCurrentAmps;
            std::string vendor;
            std::string model;
            int numberOfConnectors;
            std::string firmwareVersion;

            CHARGELAB_JSON_INTRUSIVE(
                    DeviceConfigInfo,
                    centralSystemUrl,
                    chargePointId,
                    ocppProtocol,
                    securityProfile,
                    clearCertificates,
                    serialNumber,
                    maxCurrentAmps,
                    vendor,
                    model,
                    numberOfConnectors,
                    firmwareVersion
            )
        };

        struct ConnectorSettings {
            std::optional<int> connectorId;
            std::optional<bool> vehicleConnected;
            std::optional<bool> suspendedByVehicle;
            std::optional<bool> suspendedByCharger;
            std::optional<bool> allowWebsocketConnection;
            CHARGELAB_JSON_INTRUSIVE(ConnectorSettings, connectorId, vehicleConnected, suspendedByVehicle, suspendedByCharger, allowWebsocketConnection)
        };

        struct ConnectorStatusInfo {
            chargelab::charger::ConnectorStatus connectorStatus;
            std::vector<ocpp1_6::SampledValue> meterValues1_6;
            std::vector<ocpp2_0::SampledValueType> meterValues2_0;
            ConnectorSettings connectorSettings;
            ocpp1_6::ChargePointStatus chargePointStatus1_6;
            CHARGELAB_JSON_INTRUSIVE(ConnectorStatusInfo, connectorStatus, meterValues1_6, meterValues2_0, connectorSettings, chargePointStatus1_6)
        };

        struct ChargerInfo {
            std::string serialNumber;
            std::string firmwareVersion;
            CHARGELAB_JSON_INTRUSIVE(ChargerInfo, serialNumber, firmwareVersion);
        };

        struct ChargerInfoResponse {
            ChargerInfo info;
            CHARGELAB_JSON_INTRUSIVE(ChargerInfoResponse, info);
        };

        struct SimulateRfidInteraction {
            std::string idToken;
            ocpp2_0::IdTokenEnumType tokenType;
            CHARGELAB_JSON_INTRUSIVE(SimulateRfidInteraction, idToken, tokenType);
        };

        struct WifiNetwork {
            std::string ssid;
            std::string rssi;
            std::string encryption;
            CHARGELAB_JSON_INTRUSIVE(WifiNetwork, ssid, rssi, encryption);
        };

        struct SetupPayload {
            std::string ssid;
            std::string password;
            std::string networkUrl;
            std::string ocppId;
            ocpp2_0::OCPPVersionEnumType ocppProtocol;
            CHARGELAB_JSON_INTRUSIVE(SetupPayload, ssid, password, networkUrl, ocppId, ocppProtocol);
        };

        struct OcppParametersInfo {
            ocpp1_6::GetConfigurationRsp rsp;
            CHARGELAB_JSON_INTRUSIVE(OcppParametersInfo, rsp)
        };

        struct OcppParameterSetting {
            std::string key;
            std::string value;
            CHARGELAB_JSON_INTRUSIVE(OcppParameterSetting, key, value)
        };
    }

    /**
     * Notes:
     * Increasing the HTTPD_MAX_REQ_HDR_LEN from 512 (default) to 1024 is recommended. The limit was hit on Chrome
     * browsers.
     */
    class PortalDemo : public PureService {
    private:
        static constexpr int kStartupDelayMillis = 10*1000;// 500;
        static constexpr int kMaxPostContentLengthBytes = 5*1024;

    public:
        PortalDemo(
                std::shared_ptr<PlatformESP> platform,
                std::shared_ptr<ResetModule> reset,
                std::shared_ptr<ConnectorStatusModule> connector_status_module,
                std::shared_ptr<ConfigurationModule> configuration_module,
                std::shared_ptr<StationTestEsp32> station
        )
                : platform_(std::move(platform)),
                  reset_(std::move(reset)),
                  connector_status_module_(std::move(connector_status_module)),
                  configuration_module_(std::move(configuration_module)),
                  station_(std::move(station))

        {
            CHARGELAB_LOG_MESSAGE(debug) << "Setting up portal";
            settings_ = platform_->getSettings();

            for (auto const& method : std::vector<httpd_method_t>{HTTP_GET, HTTP_POST}) {
                handlers_.resize(handlers_.size()+1);
                auto& x = handlers_.back();
                x.uri = "*";
                x.method = method;
                x.handler = onRequest;
                x.user_ctx = (void *)this;
            }
        }

        ~PortalDemo() {
            stopServers();
        }

    private:
        void runUnconditionally() override {
            if (!initialized_) {
                if (!start_ts_.has_value())
                    start_ts_ = platform_->steadyClockNow();

                auto const elapsed = platform_->steadyClockNow() - start_ts_.value();
                if (elapsed < kStartupDelayMillis)
                    return;

                startServerHttps();
                startServerHttp();
                initialized_ = true;
            }
        }

    private:
        void startServerHttps() {
            extern const unsigned char _binary_servercert_pem_start[]     asm("_binary_servercert_pem_start");
            extern const unsigned char _binary_servercert_pem_end[]       asm("_binary_servercert_pem_end");
            extern const unsigned char _binary_prvtkey_pem_start[]        asm("_binary_prvtkey_pem_start");
            extern const unsigned char _binary_prvtkey_pem_end[]          asm("_binary_prvtkey_pem_end");

            if (server_https_ != nullptr)
                return;

            CHARGELAB_LOG_MESSAGE(info) << "Starting HTTPS server...";
            httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
            {
                config.servercert = _binary_servercert_pem_start;
                config.servercert_len = _binary_servercert_pem_end - _binary_servercert_pem_start;
            }
            {
                config.prvtkey_pem = _binary_prvtkey_pem_start;
                config.prvtkey_len = _binary_prvtkey_pem_end - _binary_prvtkey_pem_start;
            }

            config.httpd.uri_match_fn = httpd_uri_match_wildcard;
            config.httpd.lru_purge_enable = true;
            config.httpd.stack_size += 1024;
            CHARGELAB_LOG_MESSAGE(info) << "Using stack size: " << config.httpd.stack_size;
            ESP_ERROR_CHECK(httpd_ssl_start(&server_https_, &config));
            for (auto& x : handlers_)
                httpd_register_uri_handler(server_https_, &x);
        }

        void startServerHttp() {
            if (server_http_ != nullptr)
                return;

            CHARGELAB_LOG_MESSAGE(info) << "Starting HTTP server...";
            httpd_config_t config = HTTPD_DEFAULT_CONFIG();
            config.uri_match_fn = httpd_uri_match_wildcard;
            config.lru_purge_enable = true;
            config.stack_size += 1024;
            CHARGELAB_LOG_MESSAGE(info) << "Using stack size: " << config.stack_size;

            ESP_ERROR_CHECK(httpd_start(&server_http_, &config));
            for (auto& x : handlers_)
                httpd_register_uri_handler(server_http_, &x);
        }

        void stopServers() {
            if (server_http_ != nullptr) {
                ESP_ERROR_CHECK(httpd_stop(server_http_));
                server_http_ = nullptr;
            }
            if (server_https_ != nullptr) {
                ESP_ERROR_CHECK(httpd_stop(server_https_));
                server_https_ = nullptr;
            }
        }

        std::optional<std::string> readQueryString(httpd_req_t *req) {
            std::string query(512, '\0');
            if (httpd_req_get_url_query_str(req, query.data(), query.size()) != ESP_OK)
                return std::nullopt;

            query.resize(strlen(query.c_str()));
            return query;
        }

        std::optional<std::string> readQueryParameter(std::optional<std::string> const& query, std::string const& key) {
            if (!query.has_value())
                return std::nullopt;

            std::string value(query->size(), '\0');
            if (httpd_query_key_value(query->data(), key.c_str(), value.data(), value.size()) != ESP_OK)
                return std::nullopt;

            value.resize(strlen(value.c_str()));
            return uri::decodeUriComponent(value);
        }

        void onGet(httpd_req_t *req) {
            // Note: as extern variables the uniqueness of the name/combination is important. These can't be moved into
            //       local variable definitions within the subsequent if blocks with shared names (start/end for
            //       example); leaving these are verbose explicit names here to limit the risk of conflicting
            //       definitions.
            extern const char _binary_web_favicon_ico_gz_start[]         asm("_binary_web_favicon_ico_gz_start");
            extern const char _binary_web_favicon_ico_gz_end[]           asm("_binary_web_favicon_ico_gz_end");
            extern const char _binary_web_index_html_gz_start[]          asm("_binary_web_index_html_gz_start");
            extern const char _binary_web_index_html_gz_end[]            asm("_binary_web_index_html_gz_end");
            extern const char _binary_web_setup_html_gz_start[]           asm("_binary_web_setup_html_gz_start");
            extern const char _binary_web_setup_html_gz_end[]             asm("_binary_web_setup_html_gz_end");
            extern const char _binary_web_config_html_gz_start[]         asm("_binary_web_config_html_gz_start");
            extern const char _binary_web_config_html_gz_end[]           asm("_binary_web_config_html_gz_end");
            extern const char _binary_web_control_html_gz_start[] asm("_binary_web_control_html_gz_start");
            extern const char _binary_web_control_html_gz_end[]   asm("_binary_web_control_html_gz_end");

            std::string const uri = req->uri;
            CHARGELAB_LOG_MESSAGE(info) << "Requested URI was: " << uri;

            if (uri == "" || uri == "/" || uri == "/index.html" || uri == "/home") {
                serveGzipHtml(req, _binary_web_index_html_gz_start, _binary_web_index_html_gz_end);
            } else if (uri == "/setup") {
                serveGzipHtml(req, _binary_web_setup_html_gz_start, _binary_web_setup_html_gz_end);
            } else if (uri == "/config") {
                serveGzipHtml(req, _binary_web_config_html_gz_start, _binary_web_config_html_gz_end);
            } else if (uri == "/control") {
                serveGzipHtml(req, _binary_web_control_html_gz_start, _binary_web_control_html_gz_end);
            } else if (uri == "/networks") {
                std::optional<std::vector<detail::WifiNetwork>> networks;
                for (int i=0; i < 10; i++) {
                    networks = scanNetworks();
                    if (networks.has_value())
                        break;

                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(10ms);
                }

                if (networks.has_value()) {
                    auto const content = write_json_to_string(networks);
                    httpd_resp_set_type(req, "application/json");
                    httpd_resp_send(req, content.data(), content.size());
                } else {
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, nullptr);
                }
            } else if (uri == "/req.html") {
            } else if (uri == "/chargerInfo") {
                detail::ChargerInfoResponse response = {
                        detail::ChargerInfo {
                                settings_->ChargerSerialNumber.getValue(),
                                settings_->ChargerFirmwareVersion.getValue()
                        }
                };

                auto const text = write_json_to_string(response);
                httpd_resp_set_type(req, "application/json");
                httpd_resp_send(req, text.data(), text.size());
            } else if (uri == "/configure") {
            } else if (uri == "/logs") {
            } else if (uri == "/getDeviceConfiguration") {
                detail::DeviceConfigInfo response {};
                response.securityProfile = settings_->SecurityProfile.getValue();
                response.clearCertificates = false;

                auto const& slot = settings_->NetworkConnectionProfiles.getValue(0);
                if (slot.has_value())
                    response.centralSystemUrl = slot->ocppCsmsUrl.value();

                response.chargePointId = settings_->ChargePointId.getValue();
                response.ocppProtocol = settings_->OcppProtocol.getValue();
                response.serialNumber = settings_->ChargerSerialNumber.getValue();
                response.maxCurrentAmps = "40";
                response.vendor = settings_->ChargerVendor.getValue();
                response.model = settings_->ChargerModel.getValue();
                response.firmwareVersion = settings_->ChargerFirmwareVersion.getValue();
                response.numberOfConnectors = station_->getConnectorMetadata().size();

                auto const text = write_json_to_string(response);
                httpd_resp_set_type(req, "application/json");
                httpd_resp_send(req, text.data(), text.size());
            } else if (uri == "/logs") {
            } else if (uri.find("/getConnectorStatus") == 0) {
                auto const query = readQueryString(req);
                int connectorId = 1;
                {
                    auto const text = readQueryParameter(query, "connectorId");
                    if (text.has_value()) {
                        auto const parsed = string::ToInteger(text.value());
                        if (parsed.has_value())
                            connectorId = parsed.value();
                    }
                }

                CHARGELAB_LOG_MESSAGE(info) << "Connector ID was: " << connectorId;
                auto const evse = station_->lookupConnectorId1_6(connectorId);
                if (!evse.has_value()) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Connector ID doesn't exist";
                    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, nullptr);
                    return;
                }

                auto const status = station_->pollConnectorStatus(evse.value());
                if (!status.has_value()) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Connector status was empty";
                    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, nullptr);
                    return;
                }

                detail::ConnectorSettings settings {};
                settings.vehicleConnected = status->vehicle_connected;
                settings.suspendedByCharger = status->suspended_by_charger;
                settings.suspendedByVehicle = status->suspended_by_vehicle;

                auto status1_6_map = connector_status_module_->getChargePointStatus1_6();
                auto it = status1_6_map.find(1);  // use default connector 1 for now

                detail::ConnectorStatusInfo response {
                        status.value(),
                        station_->pollMeterValues1_6(evse.value()),
                        station_->pollMeterValues2_0(evse.value()),
                        settings,
                        it != status1_6_map.end() ? it->second : ocpp1_6::ChargePointStatus{ocpp1_6::ChargePointStatus::kValueNotFoundInEnum}
                };

                auto const text = write_json_to_string(response);
                httpd_resp_set_type(req, "application/json");
                httpd_resp_send(req, text.data(), text.size());
            } else if (uri == "/getOcppParameters") {  // ocpp 1.6 configuration parameter
                detail::OcppParametersInfo response {};

                auto getConfigurationRsp = configuration_module_->getConfiguration(ocpp1_6::GetConfigurationReq{});
                if (getConfigurationRsp) {
                    if (std::holds_alternative<ocpp1_6::GetConfigurationRsp>(getConfigurationRsp.value())) {
                        response.rsp = std::move(std::get<ocpp1_6::GetConfigurationRsp>(std::move(getConfigurationRsp.value())));
                    }
                }

                auto const text = write_json_to_string(response);
                httpd_resp_set_type(req, "application/json");
                httpd_resp_send(req, text.data(), text.size());
            } else if (uri == "/favicon.ico") {
                serveGzipContent(req, "image/vnd.microsoft.icon", _binary_web_favicon_ico_gz_start, _binary_web_favicon_ico_gz_end);
            } else {
                httpd_resp_set_type(req, "text/html");
                httpd_resp_send(req, "<h1>Not found</h1>", HTTPD_RESP_USE_STRLEN);
            }
        }

        void onPost(httpd_req_t *req) {
            CHARGELAB_LOG_MESSAGE(info) << "Received POST request on: " << req->uri;
            std::string payload;
            if (req->content_len > 0) {
                if (req->content_len > kMaxPostContentLengthBytes) {
                    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, nullptr);
                    return;
                }

                payload.resize(req->content_len);
                auto ret = httpd_req_recv(req, payload.data(), payload.size());
                if (ret < 0 || ret != payload.size()) {
                    if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                        httpd_resp_send_408(req);
                        return;
                    } else {
                        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, nullptr);
                        return;
                    }
                }
            }
            CHARGELAB_LOG_MESSAGE(info) << "Payload was: " << payload;

            std::string const uri = req->uri;
            if (uri == "/submitSetup") {
                onPostSetupSubmit(payload, req);
            } else if (uri == "/updateDeviceConfiguration") {
                onPostUpdateDeviceConfiguration(payload, req);
            } else if (uri == "/updateConnectorSettings") {
                onPostUpdateConnectorSettings(payload, req);
            } else if (uri == "/simulateRfidInteraction") {
                onPostSimulateRfidInteraction(payload, req);
            } else if (uri == "/updateOcppParameter") {
                onPostUpdateOcppParameter(payload, req);
            } else if (uri == "/reboot") {
                onPostReboot(payload, req);
            }
            else {
                httpd_resp_set_type(req, "text/html");
                httpd_resp_send(req, "<h1>Hello Secure World - Post!</h1>", HTTPD_RESP_USE_STRLEN);
            }
        }

        void onPostSetupSubmit(std::string const& payload, httpd_req_t *req) {
            auto const parsed = read_json_from_string<detail::SetupPayload>(payload);
            if (!parsed.has_value()) {
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, nullptr);
                return;
            }

            auto const& settings = parsed.value();
            settings_->WifiSSID.forceValue(settings.ssid);
            settings_->WifiPassword.forceValue(settings.password);
            settings_->SecurityProfile.setValue(0);
            settings_->NetworkConnectionProfiles.forceValue(0, ocpp2_0::NetworkConnectionProfileType {
                    // TODO: Need to set OCPP version from a field
                    settings.ocppProtocol,
                    ocpp2_0::OCPPTransportEnumType::kJSON,
                    settings.networkUrl,
                    settings_->DefaultMessageTimeout.getValue(),
                    0,
                    chargelab::ocpp2_0::OCPPInterfaceEnumType::kWireless0
            });
            settings_->NetworkConfigurationPriority.forceValue("0");
            settings_->ChargePointId.forceValue(settings.ocppId);

            reset_->resetImmediately(ocpp2_0::BootReasonEnumType::kLocalReset);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, "{}", HTTPD_RESP_USE_STRLEN);
        }

        void onPostUpdateDeviceConfiguration(std::string const& payload, httpd_req_t *req) {
            auto const parsed = read_json_from_string<detail::DeviceConfigInfo>(payload);
            if (!parsed.has_value()) {
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, nullptr);
                return;
            }

            // Clear existing networking profiles
            for (int i=0; i < settings_->NetworkConnectionProfiles.size(); i++)
                settings_->NetworkConnectionProfiles.forceValue(i, std::nullopt);

            auto const& settings = parsed.value();
            settings_->SecurityProfile.setValue(settings.securityProfile);
            settings_->NetworkConnectionProfiles.forceValue(0, ocpp2_0::NetworkConnectionProfileType {
                    // TODO: Need to set OCPP version from a field
                    settings.ocppProtocol == "OCPP20" ? ocpp2_0::OCPPVersionEnumType::kOCPP20 :
                    ocpp2_0::OCPPVersionEnumType::kOCPP16,
                    ocpp2_0::OCPPTransportEnumType::kJSON,
                    settings.centralSystemUrl,
                    settings_->DefaultMessageTimeout.getValue(),
                    settings.securityProfile,
                    chargelab::ocpp2_0::OCPPInterfaceEnumType::kWireless0
            });
            settings_->NetworkConfigurationPriority.forceValue("0");
            settings_->ChargePointId.forceValue(settings.chargePointId);
            settings_->ChargerSerialNumber.setValue(settings.serialNumber);
            settings_->ChargerVendor.setValue(settings.vendor);
            settings_->ChargerModel.setValue(settings.model);
            settings_->ChargerFirmwareVersion.setValue(settings.firmwareVersion);

            if (settings.numberOfConnectors != (int)station_->getConnectorMetadata().size()) {
                station_->updateState([&](detail::SimulatorState& state) {
                    state.evse_metadata.clear();
                    state.connector_metadata.clear();
                    state.connector_state.clear();

                    state.station_metadata = charger::StationMetadata {
                                1
                    };

                    for (int i=1; i <= settings.numberOfConnectors; i++) {
                        state.evse_metadata[ocpp2_0::EVSEType {i}] = charger::EvseMetadata {
                                1,
                                10560.0
                        };
                        state.connector_metadata[ocpp2_0::EVSEType {i, 1}] = charger::ConnectorMetadata {
                                1,
                                "cType1",
                                1,
                                10560.0,
                                48.0
                        };
                    }
                });
            }

            if (settings.clearCertificates)
                platform_->removeCertificatesIf([](detail::SavedCertificate const&) {return true;});

            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, "{\"successful\":true}", HTTPD_RESP_USE_STRLEN);
        }

        void onPostUpdateConnectorSettings(std::string const& payload, httpd_req_t *req) {
            auto const parsed = read_json_from_string<detail::ConnectorSettings>(payload);
            if (!parsed.has_value()) {
                CHARGELAB_LOG_MESSAGE(error) << "Failed parsing ConnectorSettings from: " << payload;
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, nullptr);
                return;
            }

            auto const& settings = parsed.value();
            if (settings.allowWebsocketConnection.has_value())
                platform_->setDisableWebsocketConnection(!settings.allowWebsocketConnection.value());

            station_->updateState([&](detail::SimulatorState& state) {
                for (auto const& entry : state.connector_metadata) {
                    if (settings.connectorId.has_value() && entry.second.connector_id1_6 != settings.connectorId.value())
                        continue;

                    auto& connector_state = state.connector_state[entry.first];
                    if (settings.vehicleConnected.has_value())
                        connector_state.vehicle_connected = settings.vehicleConnected.value();
                    if (settings.suspendedByVehicle.has_value())
                        connector_state.suspended_by_vehicle = settings.suspendedByVehicle.value();
                    if (settings.suspendedByCharger.has_value())
                        connector_state.suspended_by_charger = settings.suspendedByCharger.value();
                }
            });

            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, "{}", HTTPD_RESP_USE_STRLEN);
        }

        void onPostSimulateRfidInteraction(std::string const& payload, httpd_req_t *req) {
            auto const parsed = read_json_from_string<detail::SimulateRfidInteraction>(payload);
            if (!parsed.has_value()) {
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, nullptr);
                return;
            }

            auto const data = parsed.value();
            station_->simulateTap(
                    ocpp1_6::IdToken {data.idToken},
                    ocpp2_0::IdTokenType {data.idToken, data.tokenType},
                    1000
            );
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, "{}", HTTPD_RESP_USE_STRLEN);
        }

        void onPostUpdateOcppParameter(std::string const& payload, httpd_req_t *req) {
            auto const parsed = read_json_from_string<detail::OcppParameterSetting>(payload);
            if (!parsed.has_value()) {
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, nullptr);
                return;
            }

            auto const setting = parsed.value();
            auto changeConfigRsp = configuration_module_->changeConfiguration(ocpp1_6::ChangeConfigurationReq {
                    setting.key, setting.value }, true);

            if (changeConfigRsp) {
                if (std::holds_alternative<ocpp1_6::ChangeConfigurationRsp>(changeConfigRsp.value())) {
                    auto status = std::get<ocpp1_6::ChangeConfigurationRsp>(std::move(changeConfigRsp.value()));

                    CHARGELAB_LOG_MESSAGE(info) << "change configuration, key: " << setting.key << ", return: " << status;

                    httpd_resp_set_type(req, "application/json");
                    httpd_resp_send(req, write_json_to_string(status).data(), HTTPD_RESP_USE_STRLEN);
                    return;
                }
            }
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, nullptr);
        }

        void onPostReboot(std::string const& payload, httpd_req_t *req) {
            reset_->resetImmediately(ocpp2_0::BootReasonEnumType::kLocalReset);
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, "{}", HTTPD_RESP_USE_STRLEN);
        }

        void serveGzipHtml(httpd_req_t *req, char const* start, char const* end) {
            auto length = end - start;
            httpd_resp_set_type(req, "text/html");
            httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
            httpd_resp_send(req, start, length);
        }

        void serveGzipContent(httpd_req_t *req, char const* content_type, char const* start, char const* end) {
            auto length = end - start;
            httpd_resp_set_type(req, content_type);
            httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
            httpd_resp_send(req, start, length);
        }

    private:
        static esp_err_t onRequest(httpd_req_t *req) {
            if (req == nullptr) {
                CHARGELAB_LOG_MESSAGE(warning) << "Unexpected null req";
                return ESP_FAIL;
            }

            auto const this_ptr = (PortalDemo*)req->user_ctx;
            if (this_ptr == nullptr) {
                CHARGELAB_LOG_MESSAGE(warning) << "Unexpected null user_ctx";
                return ESP_FAIL;
            }

            switch (req->method) {
                default:
                    CHARGELAB_LOG_MESSAGE(warning) << "Unexpected request method: " << req->method;
                    return ESP_FAIL;

                case HTTP_GET:
                    this_ptr->onGet(req);
                    break;

                case HTTP_POST:
                    this_ptr->onPost(req);
                    break;
            }

            return ESP_OK;
        }

        std::optional<std::vector<detail::WifiNetwork>> scanNetworks()
        {
            std::vector<detail::WifiNetwork> result;
            std::vector<wifi_ap_record_t> ap_info;
            ap_info.resize(20);

            uint16_t number = ap_info.size();
            auto scan_start_result = esp_wifi_scan_start(nullptr, true);
            if (scan_start_result != ESP_OK) {
                CHARGELAB_LOG_MESSAGE(warning) << "wifi scan start failed, error code:" << scan_start_result;
                CHARGELAB_LOG_MESSAGE(warning) << "ESP_ERR_WIFI_NOT_INIT = " << ESP_ERR_WIFI_NOT_INIT;
                CHARGELAB_LOG_MESSAGE(warning) << "ESP_ERR_WIFI_NOT_STARTED = " << ESP_ERR_WIFI_NOT_STARTED;
                CHARGELAB_LOG_MESSAGE(warning) << "ESP_ERR_WIFI_TIMEOUT = " << ESP_ERR_WIFI_TIMEOUT;
                CHARGELAB_LOG_MESSAGE(warning) << "ESP_ERR_WIFI_STATE = " << ESP_ERR_WIFI_STATE;
                return std::nullopt;
            }
            auto get_ap_result = esp_wifi_scan_get_ap_records(&number, ap_info.data());
            if (get_ap_result != ESP_OK) {
                CHARGELAB_LOG_MESSAGE(warning) << "wifi scan get ap records failed, error code: " << get_ap_result;
                return std::nullopt;
            }

            for (int i = 0; i < ap_info.size() && i < number; i++) {
                detail::WifiNetwork network {};
                network.ssid = (char const*)ap_info[i].ssid;
                network.rssi = std::to_string(ap_info[i].rssi);
                network.encryption = getAuthModeText(ap_info[i].authmode);
                result.push_back(std::move(network));
            }

            return result;
        }

        std::string getAuthModeText(int authmode)
        {
            switch (authmode) {
                case WIFI_AUTH_OPEN: return "WIFI_AUTH_OPEN";
                case WIFI_AUTH_WEP: return "WIFI_AUTH_WEP";
                case WIFI_AUTH_WPA_PSK: return "WIFI_AUTH_WPA_PSK";
                case WIFI_AUTH_WPA2_PSK: return "WIFI_AUTH_WPA2_PSK";
                case WIFI_AUTH_WPA_WPA2_PSK: return "WIFI_AUTH_WPA_WPA2_PSK";
                case WIFI_AUTH_WPA2_ENTERPRISE: return "WIFI_AUTH_WPA2_ENTERPRISE";
                case WIFI_AUTH_WPA3_PSK: return "WIFI_AUTH_WPA3_PSK";
                case WIFI_AUTH_WPA2_WPA3_PSK: return "WIFI_AUTH_WPA2_WPA3_PSK";
                default: return "WIFI_AUTH_UNKNOWN";
            }
        }

    private:
        std::shared_ptr<PlatformESP> platform_;
        std::shared_ptr<ResetModule> reset_;
        std::shared_ptr<ConnectorStatusModule> connector_status_module_;
        std::shared_ptr<ConfigurationModule> configuration_module_;
        std::shared_ptr<StationTestEsp32> station_;
        std::shared_ptr<Settings> settings_;

        std::vector<httpd_uri_t> handlers_;
        std::optional<SteadyPointMillis> start_ts_ = std::nullopt;
        bool initialized_ = false;

        httpd_handle_t server_https_ = nullptr;
        httpd_handle_t server_http_ = nullptr;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_PORTAL_DEMO_H
