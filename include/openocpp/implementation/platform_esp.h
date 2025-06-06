#ifndef CHARGELAB_OPEN_FIRMWARE_PLATFORM_ESP_H
#define CHARGELAB_OPEN_FIRMWARE_PLATFORM_ESP_H

#include "openocpp/common/settings.h"
#include "openocpp/helpers/chrono.h"
#include "openocpp/interface/component/system_interface.h"
#include "openocpp/interface/element/flash_block_interface.h"
#include "openocpp/interface/platform_interface.h"
#include "openocpp/common/storage.h"
#include "openocpp/common/ring_buffer.h"
#include "openocpp/common/macro.h"
#include "openocpp/common/serialization.h"
#include "openocpp/helpers/set.h"
#include "openocpp/helpers/uri.h"

#include <memory>
#include <atomic>
#include <optional>
#include <functional>

#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "esp_websocket_client.h"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "esp_netif_sntp.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509.h"
#include "mbedtls/oid.h"

namespace chargelab {
    namespace detail {
        class ReportHeapGlobals {
        public:
            static inline std::atomic<int> gMinFreeHeap = -1;
            static inline std::atomic<int> gMaxFreeHeap = -1;
        };

        class CustomReportHeapGlobals {
        public:
            static inline std::atomic<int> gMinFreeHeap = -1;
            static inline std::atomic<int> gMaxFreeHeap = -1;
        };
    }
}

void esp_heap_trace_alloc_hook(void* ptr, size_t size, uint32_t caps)
{
    auto const free_heap = esp_get_free_heap_size();

    {
        using globals = chargelab::detail::ReportHeapGlobals;
        if (globals::gMinFreeHeap < 0 || free_heap < globals::gMinFreeHeap)
            globals::gMinFreeHeap = free_heap;
        if (globals::gMaxFreeHeap < 0 || free_heap > globals::gMaxFreeHeap)
            globals::gMaxFreeHeap = free_heap;
    }

    {
        using globals = chargelab::detail::CustomReportHeapGlobals;
        if (globals::gMinFreeHeap < 0 || free_heap < globals::gMinFreeHeap)
            globals::gMinFreeHeap = free_heap;
        if (globals::gMaxFreeHeap < 0 || free_heap > globals::gMaxFreeHeap)
            globals::gMaxFreeHeap = free_heap;
    }
}
void esp_heap_trace_free_hook(void* ptr)
{
    auto const free_heap = esp_get_free_heap_size();

    {
        using globals = chargelab::detail::ReportHeapGlobals;
        if (globals::gMinFreeHeap < 0 || free_heap < globals::gMinFreeHeap)
            globals::gMinFreeHeap = free_heap;
        if (globals::gMaxFreeHeap < 0 || free_heap > globals::gMaxFreeHeap)
            globals::gMaxFreeHeap = free_heap;
    }

    {
        using globals = chargelab::detail::CustomReportHeapGlobals;
        if (globals::gMinFreeHeap < 0 || free_heap < globals::gMinFreeHeap)
            globals::gMinFreeHeap = free_heap;
        if (globals::gMaxFreeHeap < 0 || free_heap > globals::gMaxFreeHeap)
            globals::gMaxFreeHeap = free_heap;
    }
}

namespace chargelab {
    class PlatformESP;

    enum class CertificatesFileVersion : int {};
    enum class SettingsFileVersion : int {};

    inline void reportHeapUsageCustom(std::string const& tag) {
        using globals = chargelab::detail::CustomReportHeapGlobals;

        int free_heap = esp_get_free_heap_size();
        int free_heap_minimum = esp_get_minimum_free_heap_size();
        int free_heap_max_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

        int free_heap_minimum_since_last_report = globals::gMinFreeHeap;
        int free_heap_maximum_since_last_report = globals::gMaxFreeHeap;
        globals::gMinFreeHeap = -1;
        globals::gMaxFreeHeap = -1;

        CHARGELAB_LOG_MESSAGE(info) << tag << " Free heap: " << free_heap
                                    << " Minimum free heap: " << free_heap_minimum
                                    << " Free max block heap: " << free_heap_max_block
                                    << " Maximum free heap since last report: " << free_heap_maximum_since_last_report
                                    << " Minimum free heap since last report: " << free_heap_minimum_since_last_report
                                    << " (" << free_heap - free_heap_minimum_since_last_report << ")";
    }

    namespace detail {
        struct SavedCertificate {
            ocpp2_0::GetCertificateIdUseEnumType certificateType;
            ocpp2_0::CertificateHashDataType certificateHashData;
            ocpp2_0::IdentifierStringPrimitive<128> subjectNameHash;
            std::string certificate;
            CHARGELAB_JSON_INTRUSIVE(SavedCertificate, certificateType, certificateHashData, subjectNameHash, certificate)
        };

        struct StaticPlatformESP {
            static constexpr const char* kSpiffsPathPrefix = "/spiffs";
            static constexpr const char* kSettingsFile = "settings.json";
            static constexpr const char* kCertificatesFile = "certificates.json";
            static constexpr const CertificatesFileVersion kCertificatesFileVersion = (CertificatesFileVersion)1;
            static const inline mbedtls_x509_crt kDummyCert {};

            // Note: changing this would require re-computing the saved certificate hash data (not currently supported)
            static constexpr mbedtls_md_type_t kHashType = MBEDTLS_MD_SHA256;

            static inline std::atomic<bool> gSystemClockSet = false;
            static inline std::atomic<bool> gInvalidCsmsCertificate = false;
        };

        inline chargelab::SystemTimeMillis systemClockNow() {
            auto const now = std::chrono::system_clock::now();
            auto const millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            return static_cast<chargelab::SystemTimeMillis>(millis);
        }

        inline chargelab::SteadyPointMillis steadyClockNow() {
            auto const now = std::chrono::steady_clock::now();
            auto const millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            return static_cast<chargelab::SteadyPointMillis>(millis);
        }
        
        inline int getDelaySeconds(Settings const& settings, int failed_connection_attempts) {
            auto delay_seconds = settings.NetworkConnectionRetryBackOffWaitMinimum.getValue();
            if (failed_connection_attempts > 0) {
                delay_seconds *= std::pow(2, std::min(
                        failed_connection_attempts - 1,
                        settings.NetworkConnectionRetryBackOffRepeatTimes.getValue()
                ));
            }

            std::default_random_engine random_engine {std::random_device{}()};
            std::uniform_int_distribution<int64_t> distribution(0, settings.NetworkConnectionRetryBackOffRandomRange.getValue());
            delay_seconds += distribution(random_engine);
            return delay_seconds;
        }
    }

    namespace certs {
        SystemTimeMillis convertToTimestamp(mbedtls_x509_time const& time) {
            std::tm tt {};
            tt.tm_year = time.year - 1900;
            tt.tm_mon = time.mon - 1;
            tt.tm_mday = time.day;
            tt.tm_hour = time.hour;
            tt.tm_min = time.min;
            tt.tm_sec = time.sec;

            return static_cast<SystemTimeMillis>(((int64_t)chrono::timegm2(&tt))*1000);
        }

        class MbedTlsCertificateResource {
        public:
            MbedTlsCertificateResource() {
                mbedtls_x509_crt_init(&cert);
            }

            ~MbedTlsCertificateResource() {
                mbedtls_x509_crt_free(&cert);
            }

        public:
            mbedtls_x509_crt cert;
        };

        std::optional<ocpp2_0::HashAlgorithmEnumType> toHashAlgorithm(mbedtls_md_type_t const& mbedtls_type) {
            switch (mbedtls_type) {
                case MBEDTLS_MD_SHA256: return ocpp2_0::HashAlgorithmEnumType::kSHA256;
                case MBEDTLS_MD_SHA384: return ocpp2_0::HashAlgorithmEnumType::kSHA384;
                case MBEDTLS_MD_SHA512: return ocpp2_0::HashAlgorithmEnumType::kSHA512;
                default: return std::nullopt;
            }
        }

        bool verifyCertificate(
                mbedtls_x509_crt const* child,
                mbedtls_x509_crt const* ca,
                std::optional<SystemTimeMillis> const& now
        ) {
            // Note: CLR is not currently used/implemented
            mbedtls_x509_crl ca_clr;
            std::memset(&ca_clr, 0, sizeof(mbedtls_x509_crl));

            if (now.has_value()) {
                {
                    auto const valid_from = convertToTimestamp(child->valid_from);
                    auto const valid_to = convertToTimestamp(child->valid_to);
                    if (valid_from > now.value() || valid_to < now.value())
                        return false;
                }

                {
                    auto const valid_from = convertToTimestamp(ca->valid_from);
                    auto const valid_to = convertToTimestamp(ca->valid_to);
                    if (valid_from > now.value() || valid_to < now.value())
                        return false;
                }
            }

            std::uint32_t flags = 0;
            auto const ret = mbedtls_x509_crt_verify((mbedtls_x509_crt*)child, (mbedtls_x509_crt*)ca, &ca_clr, nullptr, &flags, nullptr, nullptr);
            return ret == 0;
        }

        bool verifyCertificate(
                MbedTlsCertificateResource const& child,
                MbedTlsCertificateResource const& ca,
                std::optional<SystemTimeMillis> const& now
        ) {
            return verifyCertificate(&child.cert, &ca.cert, now);
        }

        template <typename Visitor>
        std::optional<std::pair<CertificatesFileVersion, int>> visitCustomCertificates(Visitor&& visitor) {
            using settings = detail::StaticPlatformESP;
            auto const path = std::string{settings::kSpiffsPathPrefix} + "/" + settings::kCertificatesFile;
            chargelab::StorageFile storage {path};

            std::optional<std::pair<CertificatesFileVersion, int>> result = std::nullopt;
            storage.read([&](auto file) {
                CertificatesFileVersion version;
                if (std::fread(&version, sizeof(version), 1, file) != 1)
                    return false;
                if ((int)version < 0 || (int)version > (int)settings::kCertificatesFileVersion) {
                    CHARGELAB_LOG_MESSAGE(error) << "Bad certificate file version: " << (int)version;
                    return false;
                }

                int write_counter;
                if (std::fread(&write_counter, sizeof(write_counter), 1, file) != 1)
                    return false;

                result = std::make_pair(version, write_counter);
                while (true) {
                    if (file::is_eof_ignore_whitespace(file))
                        break;

                    auto record = file::json_read_object_from_file<detail::SavedCertificate>(file);
                    if (!record.has_value())
                        continue;

                    visitor(record.value());
                }

                return true;
            });

            return result;
        }

        inline std::optional<std::string> getMessageDigestHex(mbedtls_md_info_t const* md_info, unsigned char const* input, std::size_t ilen) {
            if (md_info == nullptr) {
                CHARGELAB_LOG_MESSAGE(warning) << "Missing message digest info";
                return std::nullopt;
            }

            std::string digest_buffer;
            digest_buffer.resize(mbedtls_md_get_size(md_info), '\0');
            auto ret = mbedtls_md(md_info, input, ilen, (unsigned char*)digest_buffer.data());
            if (ret != 0) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed calculating message digest - error: " << ret;
                return std::nullopt;
            }

            return string::ToHexString((unsigned char*)digest_buffer.data(), digest_buffer.size(), "");
        }

        inline std::optional<std::string> getCertificateKeyHash(
                mbedtls_md_info_t const* md_info,
                MbedTlsCertificateResource& cert
        ) {
            auto const len = mbedtls_pk_get_len(&cert.cert.pk);
            if (len == 0) {
                CHARGELAB_LOG_MESSAGE(error) << "Failed fetching public key length";
                return std::nullopt;
            }

            std::string buffer;
            buffer.resize(len + 256, '\0');

            auto p = (unsigned char*)&buffer[buffer.size()];
            auto start = (unsigned char*)buffer.data();
            // Note: this function writes the DER encoded payload without the tag or length prefix which is what we want
            auto ret = mbedtls_pk_write_pubkey(&p, start, &cert.cert.pk);
            if (ret <= 0) {
                CHARGELAB_LOG_MESSAGE(error) << "Failed writing public key to buffer: " << string::ToHexValue(ret);
                return std::nullopt;
            }

            return getMessageDigestHex(md_info, p, ret);
        }

        inline int customCrtVerifyCallback(void *buf, mbedtls_x509_crt* const crt, const int depth, uint32_t* const flags) {
            if (crt == nullptr)
                return MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;

            std::string info;
            info.resize(1024, '\0');
            mbedtls_x509_crt_info(info.data(), info.size()-1, "", crt);
            CHARGELAB_LOG_MESSAGE(info) << "Verifying certificate against custom roots:\n" << info.c_str();

            uint32_t flags_filtered = *flags & ~(MBEDTLS_X509_BADCERT_BAD_MD);
            if (flags_filtered != MBEDTLS_X509_BADCERT_NOT_TRUSTED) {
                CHARGELAB_LOG_MESSAGE(warning) << "Skipping verification - already failed with: " << string::ToHexValue(flags_filtered);
                detail::StaticPlatformESP::gInvalidCsmsCertificate = true;
                return 0;
            }

            // Reject wildcard certificates
            for (auto cur = &crt->subject_alt_names; cur != nullptr; cur = cur->next) {
                switch ((unsigned char) cur->buf.tag & MBEDTLS_ASN1_TAG_VALUE_MASK) {
                    case MBEDTLS_X509_SAN_DNS_NAME:
                    case MBEDTLS_X509_SAN_UNIFORM_RESOURCE_IDENTIFIER:
                        {
                            auto text = std::string {(char*)cur->buf.p, cur->buf.len};
                            if (text.find("*") != std::string::npos) {
                                CHARGELAB_LOG_MESSAGE(warning) << "Rejecting certificate with wildcard SAN: " << text;
                                detail::StaticPlatformESP::gInvalidCsmsCertificate = true;
                                return MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
                            }
                        }
                        break;

                    case MBEDTLS_X509_SAN_IP_ADDRESS:
                    default:
                        break;
                }
            }
            for (auto name = &crt->subject; name != nullptr; name = name->next) {
                if (MBEDTLS_OID_CMP(MBEDTLS_OID_AT_CN, &name->oid) != 0)
                    continue;

                auto text = std::string {(char*)name->val.p, name->val.len};
                if (text.find("*") != std::string::npos) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Rejecting certificate with wildcard CN: " << text;
                    detail::StaticPlatformESP::gInvalidCsmsCertificate = true;
                    return MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
                }
            }

            auto md_info = mbedtls_md_info_from_type(detail::StaticPlatformESP::kHashType);
            auto raw_hash = getMessageDigestHex(md_info, crt->issuer_raw.p, crt->issuer_raw.len);
            if (!raw_hash.has_value()) {
                CHARGELAB_LOG_MESSAGE(error) << "Failed calculating certificate hash";
                return MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
            }

            auto const issuerNameHash = ocpp2_0::IdentifierStringPrimitive<128> {raw_hash.value()};
            bool verified = false;
            certs::visitCustomCertificates([&](detail::SavedCertificate const& x) {
                if (verified)
                    return;
                if (x.subjectNameHash != issuerNameHash) {
                    CHARGELAB_LOG_MESSAGE(error) << "Skipping cert - subject name hash doesn't match";
                    return;
                }

                certs::MbedTlsCertificateResource holder;

                // Note: buflen is intended to include the null byte at the end of the string:
                // > buflen – The size of buf, including the terminating NULL byte in case of PEM encoded data.
                auto ret = mbedtls_x509_crt_parse(&holder.cert, (unsigned char const*)x.certificate.c_str(), x.certificate.size()+1);
                if (ret != 0) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Failed parsing certificate (ret=" << ret << "): " << x.certificate;
                    return;
                }

                std::optional<SystemTimeMillis> now;
                if (detail::StaticPlatformESP::gSystemClockSet)
                    now = detail::systemClockNow();

                if (certs::verifyCertificate(crt, &holder.cert, now)) {
                    verified = true;
                } else {
                    CHARGELAB_LOG_MESSAGE(error) << "Failed verifying cert";
                }
            });

            if (verified) {
                *flags = 0;
                return 0;
            } else {
                detail::StaticPlatformESP::gInvalidCsmsCertificate = true;
                return MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
            }
        }

        inline esp_err_t attachCustomCrtBundle(void* conf)
        {
            bool found = false;
            visitCustomCertificates([&](detail::SavedCertificate const& cert) {
                if (cert.certificateType == ocpp2_0::GetCertificateIdUseEnumType::kCSMSRootCertificate)
                    found = true;
            });

            if (found) {
                if (conf) {
                    // TODO: This should be replaced with a more appropriate call to mbedtls_ssl_conf_ca_cb, however
                    //       this doesn't appear to be supported by the version of the mbedtls library currently used.
                    //       Consider re-evaluating when upgrading the esp-idf version.

                    // Note: installing dummy cert to mirror esp_crt_bundle_attach
                    mbedtls_ssl_config *ssl_conf = (mbedtls_ssl_config *)conf;
                    mbedtls_ssl_conf_ca_chain(ssl_conf, (mbedtls_x509_crt*)&detail::StaticPlatformESP::kDummyCert, NULL);
                    mbedtls_ssl_conf_verify(ssl_conf, customCrtVerifyCallback, NULL);
                }

                return ESP_OK;
            } else {
                return esp_crt_bundle_attach(conf);
            }
        }
    }

    namespace detail {
        class WebsocketImpl;

        class StubWebsocket : public WebsocketInterface {
        public:
            StubWebsocket(std::string const& subprotocol)
                : subprotocol_(subprotocol)
            {
            }

            virtual ~StubWebsocket() {}

            bool isConnected() override {
                return false;
            }

            std::optional<std::string> getSubprotocol() override {
                return subprotocol_;
            }

            std::size_t pendingMessages() override {
                return 0;
            }

            std::optional<std::string> pollMessages() override {
                return std::nullopt;
            }

            void sendCustom(std::function<void(ByteWriterInterface&)>) override {
            }

        private:
            std::string subprotocol_;
        };

        class FlashPartition : public FlashBlockInterface {
        public:
            explicit FlashPartition(esp_partition_t const* partition) : partition_(partition)
            {
                assert(partition_ != nullptr);
            }

        public:
            bool read(std::size_t src_offset, void *dst, std::size_t size) const override {
                ESP_ERROR_CHECK(esp_partition_read(partition_, src_offset, dst, size));
                return true;
            }

            bool write(std::size_t dst_offset, void *src, std::size_t size) override {
                ESP_ERROR_CHECK(esp_partition_write(partition_, dst_offset, src, size));
                return true;
            }

            bool erase() override {
                ESP_ERROR_CHECK(esp_partition_erase_range(partition_, 0, size()));
                return true;
            }

            [[nodiscard]] virtual std::size_t size() const override {
                auto result = partition_->size;
                if ((result % partition_->erase_size) != 0) {
                    result -= (result % partition_->erase_size);
                }

                return result;
            }

        private:
            esp_partition_t const* partition_;
        };

        class WebsocketByteWriterImpl : public ByteWriterInterface {
        private:
            static const int kSendTimeoutMillis = 15*1000;
            friend class WebsocketImpl;

        public:
            explicit WebsocketByteWriterImpl(esp_websocket_client_handle_t& client)
                : client_(client)
            {
            }

            ~WebsocketByteWriterImpl() {
                if (failed_)
                    return;
                if (disconnected_ || !esp_websocket_client_is_connected(client_)) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Skipping flush on close - websocket disconnected";
                    return;
                }

                if (fragmented_) {
                    if (!flush())
                        return;

                    esp_websocket_client_send_fin(client_, kSendTimeoutMillis/portTICK_PERIOD_MS);
                } else if(index_ > 0) {
                    esp_websocket_client_send_text(client_, buffer_.data(), index_, kSendTimeoutMillis/portTICK_PERIOD_MS);
                    CHARGELAB_LOG_MESSAGE(info) << "Sending message: " << std::string{buffer_.data(), index_};
                }
            }

            void write(char const* s, std::size_t count) override {
                if (failed_)
                    return;

                while (count > 0) {
                    if (index_ >= buffer_.size()) {
                        if (!flush())
                            return;
                    }

                    auto const bytes = std::min(count, buffer_.size() - index_);
                    std::memcpy(buffer_.data() + index_, s, bytes);
                    s += bytes;
                    count -= bytes;
                    index_ += bytes;
                }
            }

        private:
            bool flush() {
                if (failed_)
                    return false;
                if (index_ == 0)
                    return true;

                if (disconnected_ || !esp_websocket_client_is_connected(client_)) {
                    failed_ = true;
                    CHARGELAB_LOG_MESSAGE(warning) << "Flush failed - websocket disconnected";
                    return false;
                }

                int result;
                if (!fragmented_) {
                    result = esp_websocket_client_send_text_partial(client_, buffer_.data(), index_, kSendTimeoutMillis/portTICK_PERIOD_MS);
                    CHARGELAB_LOG_MESSAGE(info) << "Sending message: " << std::string{buffer_.data(), index_};
                } else {
                    result = esp_websocket_client_send_cont_msg(client_, buffer_.data(), index_, kSendTimeoutMillis/portTICK_PERIOD_MS);
                    CHARGELAB_LOG_MESSAGE(info) << "Sending fragmented message: " << std::string{buffer_.data(), index_};
                }

                if (result != index_) {
                    failed_ = true;
                    CHARGELAB_LOG_MESSAGE(warning) << "Flush failed - bad result: " << result;
                    return false;
                }

                fragmented_ = true;
                index_ = 0;
                return true;
            }

        private:
            esp_websocket_client_handle_t& client_;

            std::atomic<bool> disconnected_ = false;
            bool failed_ = false;
            bool fragmented_ = false;
            std::size_t index_ = 0;
            std::array<char, 256> buffer_;
        };

        class WebsocketImpl : public WebsocketInterface {
        private:
            static const int kMaxMessageBufferBytes = 4*1024;
            static const int kMaxRingBufferSize = 4;
            static const int kTimeoutCloseMillis = 1000;
            friend class ::chargelab::PlatformESP;

        public:
            virtual ~WebsocketImpl() {
                if (!running_)
                    return;

                CHARGELAB_LOG_MESSAGE(info) << "Stopping websocket client...";
                if (esp_websocket_client_is_connected(client_)) {
                    if (esp_websocket_client_close(client_, kTimeoutCloseMillis / portTICK_PERIOD_MS) != ESP_OK)
                        CHARGELAB_LOG_MESSAGE(warning) << "Failed closing websocket client";
                } else {
                    if (esp_websocket_client_stop(client_) != ESP_OK)
                        CHARGELAB_LOG_MESSAGE(warning) << "Failed stopping websocket client";
                }

                if (esp_websocket_client_destroy(client_) != ESP_OK)
                    CHARGELAB_LOG_MESSAGE(warning) << "Failed destroying websocket client";
            };

            explicit WebsocketImpl(
                    std::shared_ptr<Settings> settings,
                    std::string const& central_system_url,
                    std::string const& charge_point_id,
                    std::string const& ocpp_protocol,
                    std::string const& basic_auth_password,
                    std::atomic<int>& failed_connection_attempts
            )
                : settings_(std::move(settings)),
                  basic_auth_username_(charge_point_id),
                  basic_auth_password_(basic_auth_password),
                  failed_connection_attempts_(failed_connection_attempts)
            {
                uri_ = central_system_url;
                if (!uri_.empty() && uri_[uri_.size()-1] != '/')
                    uri_ += '/';
                uri_ += charge_point_id;
                CHARGELAB_LOG_MESSAGE(info) << "Connecting to: " << uri_;

                uri_parts_ = uri::ParseWebsocketUri(uri_).value();
                subprotocol_ = ocpp_protocol;

                esp_websocket_client_config_t config = {};
                config.uri = uri_.c_str();
                config.host = uri_parts_.host.c_str();
                config.path = uri_parts_.path.c_str();
                config.port = uri_parts_.port;
                config.subprotocol = subprotocol_.c_str();
                config.ping_interval_sec = 10;
                config.crt_bundle_attach = certs::attachCustomCrtBundle;
                config.username = basic_auth_username_.c_str();
                config.password = basic_auth_password_.c_str();
                config.reconnect_timeout_ms = settings_->NetworkConnectionRetryBackOffWaitMinimum.getValue() * 1000;

                // Note: increasing from 4k default (WEBSOCKET_TASK_STACK) due to stack overflow verifying custom
                // certificates.
                config.task_stack = 6*1024;

                switch (uri_parts_.scheme) {
                    case uri::WebsocketScheme::kWS:
                        config.transport = WEBSOCKET_TRANSPORT_OVER_TCP;
                        break;

                    default:
                    case uri::WebsocketScheme::kWSS:
                        config.transport = WEBSOCKET_TRANSPORT_OVER_SSL;
                        break;
                }

                CHARGELAB_LOG_MESSAGE(info) << "Connecting to port: " << config.port;
                CHARGELAB_LOG_MESSAGE(info) << "Connecting using secure transport: " << (config.transport == WEBSOCKET_TRANSPORT_OVER_SSL);
                client_ = esp_websocket_client_init(&config);
                esp_websocket_register_events(client_, WEBSOCKET_EVENT_ANY, eventHandler, (void *)this);
                esp_websocket_client_start(client_);
            }

            bool isConnected() override {
                return esp_websocket_client_is_connected(client_);
            }

            std::optional<std::string> getSubprotocol() override {
                return subprotocol_;
            }

            std::size_t pendingMessages() override {
                std::lock_guard<std::mutex> lock(mutex_);
                return received_messages_.size();
            }

            std::optional<std::string> pollMessages() override {
                std::lock_guard<std::mutex> lock(mutex_);
                return received_messages_.popFront();
            }

            void sendCustom(std::function<void(ByteWriterInterface&)> payload) override {
                WebsocketByteWriterImpl writer {client_};
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    active_writer_ = &writer;
                }
                CHARGELAB_TRY {
                    payload(writer);
                } CHARGELAB_CATCH {
                    std::lock_guard<std::mutex> lock(mutex_);
                    active_writer_ = &writer;
                    CHARGELAB_RETHROW;
                }
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    active_writer_ = nullptr;
                }
            }

        private:
            static void eventHandler(void* handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
                auto this_ptr = (WebsocketImpl*)handler_args;
                esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
                CHARGELAB_LOG_MESSAGE(trace) << "Websocket event ID: " << event_id;

                bool reconnecting = false;
                switch (event_id) {
                    case WEBSOCKET_EVENT_CONNECTED:
                        CHARGELAB_LOG_MESSAGE(info) << "Websocket connected";
                        this_ptr->clearBuffer();
                        break;

                    case WEBSOCKET_EVENT_DISCONNECTED:
                        CHARGELAB_LOG_MESSAGE(info) << "Websocket disconnected";
                        this_ptr->failed_connection_attempts_++;
                        reconnecting = true;
                        {
                            std::lock_guard<std::mutex> lock(this_ptr->mutex_);
                            if (this_ptr->active_writer_ != nullptr)
                                this_ptr->active_writer_->disconnected_ = true;
                        }
                        break;

                    case WEBSOCKET_EVENT_DATA:
                        CHARGELAB_LOG_MESSAGE(trace) << "Websocket data opcode: " << (int)data->op_code;
                        if ((int)data->op_code != 0x8) {
                            // Note: delaying marking the connection as "successful" until this point to cater to TC_B_57_CS
                            this_ptr->established_connection_ = true;
                            this_ptr->failed_connection_attempts_ = 0;
                        }

                        switch ((int)data->op_code) {
                            case 0x0: // Continuation frame
                                this_ptr->addToBuffer(data->data_ptr, data->data_len);
                                if (data->payload_offset + data->data_len >= data->payload_len && data->fin) {
                                    CHARGELAB_LOG_MESSAGE(info) << "Received multi-part websocket message: " << this_ptr->buffer_;
                                    this_ptr->flushBuffer();
                                }
                                break;

                            case 0x1: // Text frame
                            case 0x2: // Binary frame
                                if (data->payload_offset <= 0)
                                    this_ptr->clearBuffer();

                                if (data->payload_offset + data->data_len < data->payload_len || !data->fin) {
                                    this_ptr->addToBuffer(data->data_ptr, data->data_len);
                                } else {
                                    this_ptr->addToBuffer(data->data_ptr, data->data_len);
                                    this_ptr->flushBuffer();
                                }

                                break;

                            case 0x9: // Ping frame
                                CHARGELAB_LOG_MESSAGE(info) << "Received websocket ping";
                                break;

                            case 0xA: // Pong frame
                                CHARGELAB_LOG_MESSAGE(info) << "Received websocket pong";
                                break;

                            default: // Control/close messages
                                break;
                        }

                        break;

                    case WEBSOCKET_EVENT_ERROR:
                        CHARGELAB_LOG_MESSAGE(info) << "Websocket error";
                        reconnecting = true;
                        break;

                    case WEBSOCKET_EVENT_FINISH:
                        CHARGELAB_LOG_MESSAGE(info) << "Websocket connection finished";
                        this_ptr->running_ = false;
                        break;
                }

                if (reconnecting) {
                    int failed = this_ptr->failed_connection_attempts_;
                    CHARGELAB_LOG_MESSAGE(error) << "failed_connection_attempts_=" << failed;
                    auto const delay_seconds = getDelaySeconds(*(this_ptr->settings_), this_ptr->failed_connection_attempts_);
                    CHARGELAB_LOG_MESSAGE(info) << "Delaying next reconnect attempt by: " << delay_seconds << " seconds";

                    if (esp_websocket_client_set_reconnect_timeout(this_ptr->client_, delay_seconds*1000) != ESP_OK)
                        CHARGELAB_LOG_MESSAGE(error) << "Failed setting next websocket reconnect timeout";
                }
            }

        private:
            void addToBuffer(char const* data, std::size_t length) {
                if (buffer_.size() + length > kMaxMessageBufferBytes) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Dropping part of websocket message - maximum message size exceeded";
                    length = kMaxMessageBufferBytes - buffer_.size();
                }
                if (length == 0)
                    return;

                auto const index = buffer_.size();
                buffer_.resize(buffer_.size() + length);
                std::memcpy(buffer_.data() + index, data, length);
            }

            void flushBuffer() {
                std::lock_guard<std::mutex> lock(mutex_);

                CHARGELAB_TRY {
                    CHARGELAB_LOG_MESSAGE(info) << "Received message: " << buffer_;
                    received_messages_.pushBack(buffer_);
                } CHARGELAB_CATCH {
                    CHARGELAB_LOG_MESSAGE(warning) << "Failed parsing websocket message: " << buffer_;
                }

                clearBuffer();
            }

            void clearBuffer() {
                std::string empty;
                std::swap(empty, buffer_);
            }

        private:
            std::shared_ptr<Settings> settings_;
            std::string basic_auth_username_;
            std::string basic_auth_password_;
            std::atomic<int>& failed_connection_attempts_;

            esp_websocket_client_handle_t client_;
            WebsocketByteWriterImpl* active_writer_ = nullptr;

            std::string uri_;
            uri::WebsocketParts uri_parts_;
            std::string subprotocol_;
            std::string buffer_;
            std::atomic<bool> running_ = true;
            std::atomic<bool> established_connection_ = false;

            std::mutex mutex_;
            RingBuffer<std::string, kMaxRingBufferSize> received_messages_;
        };

        class RestRequest : public RestConnectionInterface {
        public:
            RestRequest(RestMethod method, std::string uri)
            {
                esp_http_client_config_t config{};
                config.url = uri.c_str();
                config.crt_bundle_attach = esp_crt_bundle_attach;
                // config.event_handler = event_handler;
                // TODO: Set the user agent to something appropriate here?
                config.timeout_ms = 10000;
                client_ = esp_http_client_init(&config);

                switch (method) {
                    case RestMethod::kGet:     esp_http_client_set_method(client_, HTTP_METHOD_GET); break;
                    case RestMethod::kPost:    esp_http_client_set_method(client_, HTTP_METHOD_POST); break;
                    case RestMethod::kPut:     esp_http_client_set_method(client_, HTTP_METHOD_PUT); break;
                    case RestMethod::kPatch:   esp_http_client_set_method(client_, HTTP_METHOD_PATCH); break;
                    case RestMethod::kDelete:  esp_http_client_set_method(client_, HTTP_METHOD_DELETE); break;
                    case RestMethod::kHead:    esp_http_client_set_method(client_, HTTP_METHOD_HEAD); break;
                    case RestMethod::kOptions: esp_http_client_set_method(client_, HTTP_METHOD_OPTIONS); break;
                }
            }

            virtual ~RestRequest() {
                if (client_ != nullptr) {
                    esp_http_client_close(client_);
                    esp_http_client_cleanup(client_);
                }
            }

            void setHeader(std::string const& key, std::string const& value) override {
                if (client_ == nullptr) {
                    CHARGELAB_LOG_MESSAGE(error) << "Called after client was closed";
                    return;
                }
                if (opened_) {
                    CHARGELAB_LOG_MESSAGE(error) << "Called after open";
                    return;
                }

                esp_http_client_set_header(client_, key.c_str(), value.c_str());
            }

            bool open(std::size_t content_length) override {
                if (client_ == nullptr) {
                    CHARGELAB_LOG_MESSAGE(error) << "Called after client was closed";
                    return false;
                }

                esp_err_t err;
                if ((err = esp_http_client_open(client_, content_length)) != ESP_OK) {
                    CHARGELAB_LOG_MESSAGE(error) << "Failed opening HTTP connection: " << esp_err_to_name(err);
                    esp_http_client_close(client_);
                    esp_http_client_cleanup(client_);
                    client_ = nullptr;
                    return false;
                }

                total_read_len_ = 0;
                opened_ = true;
                return true;
            }

            bool send() override {
                if (client_ == nullptr) {
                    CHARGELAB_LOG_MESSAGE(error) << "Called after client was closed";
                    return false;
                }

                auto const content_length = esp_http_client_fetch_headers(client_);
                if (content_length < 0) {
                    CHARGELAB_LOG_MESSAGE(error) << "Failed fetching headers: " << content_length;
                    esp_http_client_close(client_);
                    esp_http_client_cleanup(client_);
                    client_ = nullptr;
                    return false;
                }

                content_length_ = (std::size_t)content_length;
                status_code_ = esp_http_client_get_status_code(client_);
                CHARGELAB_LOG_MESSAGE(debug) << "Server response was: status_code=" << status_code_ << ", content_length=" << content_length_;
                return true;
            }

            int getStatusCode() override {
                if (!status_code_.has_value()) {
                    CHARGELAB_LOG_MESSAGE(error) << "No status code present - defaulting to 0";
                    return 0;
                }

                return status_code_.value();
            }

            std::size_t getContentLength() override {
                if (!content_length_.has_value()) {
                    CHARGELAB_LOG_MESSAGE(error) << "No content length present - defaulting to 0";
                    return 0;
                }

                return content_length_.value();
            }

            int read(char *buffer, int len) override {
                if (client_ == nullptr) {
                    CHARGELAB_LOG_MESSAGE(error) << "Client was already closed";
                    return -2;
                }
                if (len < 0) {
                    CHARGELAB_LOG_MESSAGE(error) << "Bad len provided: " << len;
                    return -3;
                }
                if (!content_length_.has_value()) {
                    CHARGELAB_LOG_MESSAGE(error) << "Missing content length";
                    return -4;
                }

                if (total_read_len_ >= content_length_ || len == 0)
                    return 0;

                auto const remaining = content_length_.value() - total_read_len_;
                int read_len;

                if (len > 1) {
                    // Note: appears to overflow if the buffer size is used; might add a terminating null byte?
                    read_len = esp_http_client_read(client_, buffer, std::min((int)remaining, len-1));
                    if (read_len >= 0)
                        buffer[read_len] = '\0';
                } else {
                    std::array<char, 2> single_char_buffer {};
                    read_len = esp_http_client_read(client_, single_char_buffer.data(), single_char_buffer.size()-1);
                    if (read_len > 0)
                        buffer[0] = single_char_buffer[0];
                }

                if (read_len <= 0) {
                    CHARGELAB_LOG_MESSAGE(error) << "Failed reading data from stream - return code: " << read_len;
                    esp_http_client_close(client_);
                    esp_http_client_cleanup(client_);
                    client_ = nullptr;
                    return -1;
                }

                CHARGELAB_LOG_MESSAGE(debug) << "Read in: " << read_len << " bytes (" << remaining - read_len << " remaining)";
                total_read_len_ += read_len;
                return read_len;
            }

            int write(const char *buffer, int len) override {
                if (client_ == nullptr) {
                    CHARGELAB_LOG_MESSAGE(error) << "Client was already closed";
                    return -2;
                }
                if (len < 0) {
                    CHARGELAB_LOG_MESSAGE(error) << "Bad len provided: " << len;
                    return -3;
                }

                return esp_http_client_write(client_, buffer, len);
            }

        private:
            esp_http_client_handle_t client_;
            std::size_t total_read_len_ = 0;
            bool opened_ = false;

            std::optional<std::size_t> content_length_ = std::nullopt;
            std::optional<int> status_code_ = std::nullopt;
        };
    }

    class PlatformESP : public PlatformInterface {
    private:
        static constexpr int kHeapReportFrequencySeconds = 5;
        static constexpr int kDiskReportFrequencySeconds = 60;
        static constexpr int kWiFiStrengthReportFrequencySeconds = 60;
        static constexpr int kCertificateReportFrequency = 60;
        static constexpr int kMinPasswordLength = 8;
        static constexpr int kSpiffsMaxFiles = 5;
        static constexpr int kMaxPendingSecurityEvents = 5;

        static constexpr const char* kAccessPointIpPrefix1 = "192.168";
        static constexpr const char* kAccessPointIpAddress1 = "192.168.4.1";
        static constexpr const char* kAccessPointIpAddress2 = "10.0.0.1";
        static constexpr const char* kSpiffsPathPrefix = "/spiffs";
        static constexpr const char* kSettingsFile = "settings.json";
        static constexpr const char* kCertificatesFile = "certificates.json";

    public:
        PlatformESP()
        {
            CHARGELAB_LOG_MESSAGE(debug) << "Setting up platform";

            esp_err_t ret = nvs_flash_init();
            if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
                ESP_ERROR_CHECK(nvs_flash_erase());
                ret = nvs_flash_init();
            }
            ESP_ERROR_CHECK(ret);

            spiffs_conf_.base_path = kSpiffsPathPrefix;
            spiffs_conf_.partition_label = nullptr;
            spiffs_conf_.max_files = kSpiffsMaxFiles;
            spiffs_conf_.format_if_mount_failed = true;

            ret = esp_vfs_spiffs_register(&spiffs_conf_);
            if (ret != ESP_OK) {
                if (ret == ESP_FAIL) {
                    CHARGELAB_LOG_MESSAGE(error) << "Failed to mount or format filesystem";
                } else if (ret == ESP_ERR_NOT_FOUND) {
                    CHARGELAB_LOG_MESSAGE(error) << "Failed to find SPIFFS partition";
                } else {
                    CHARGELAB_LOG_MESSAGE(error) << "Failed to initialize SPIFFS: " << esp_err_to_name(ret);
                }
                ESP_ERROR_CHECK(ret);
            }

            CHARGELAB_LOG_MESSAGE(info) << "Performing SPIFFS check";
            ret = esp_spiffs_check(spiffs_conf_.partition_label);
            if (ret != ESP_OK) {
                CHARGELAB_LOG_MESSAGE(error) << "SPIFFS check failed";
                ESP_ERROR_CHECK(ret);
            }

            size_t total = 0, used = 0;
            ret = esp_spiffs_info(spiffs_conf_.partition_label, &total, &used);
            if (ret != ESP_OK) {
                CHARGELAB_LOG_MESSAGE(error) << "Failed to get SPIFFS partition information (" << esp_err_to_name(ret) << ") - formatting";
                esp_spiffs_format(spiffs_conf_.partition_label);
            }

            auto settingsPath = std::string(kSpiffsPathPrefix) + "/" + kSettingsFile;
            auto settingsStorage = std::make_shared<StorageFile>(std::move(settingsPath));
            settings_ = std::make_shared<Settings>(std::move(settingsStorage));
            settings_->loadFromStorage();
            reportDiskUsage();
            reportCertificateMetrics(true);

            auto list = esp_tls_get_ciphersuites_list();
            if (list != nullptr) {
                std::string ids;
                char const* ifs = "";
                while ((*list) != 0) {
                    ids += ifs;
                    ids += string::ToHexValue(*list);
                    list++;
                    ifs = ", ";
                }

                CHARGELAB_LOG_MESSAGE(info) << "Supported TLS ciphersuites: " << ids;
            }
        }

        ~PlatformESP() override {
            CHARGELAB_LOG_MESSAGE(debug) << "Deleting PlatformESP";
        }

    public:
        void resetSoft() override {
            resetHard();
        }

        chargelab::SystemTimeMillis systemClockNow() override {
            return detail::systemClockNow();
        }

        chargelab::SteadyPointMillis steadyClockNow() override {
            return detail::steadyClockNow();
        }

        void setSystemClock(SystemTimeMillis now) override {
            CHARGELAB_LOG_MESSAGE(debug) << "Setting system clock to: " << chrono::ToString(now);
            struct timeval tv {};
            tv.tv_sec = now/1000;
            tv.tv_usec = (now%1000)*1000;
            settimeofday(&tv, nullptr);

            detail::StaticPlatformESP::gSystemClockSet = true;
        }

        void resetHard() override {
            if (websocket_ != nullptr) {
                // Attempt to close the websocket connection gracefully before restarting
                if (esp_websocket_client_is_connected(websocket_->client_)) {
                    CHARGELAB_LOG_MESSAGE(info) << "Closing websocket client";
                    if (esp_websocket_client_close(websocket_->client_, 1000 / portTICK_PERIOD_MS) != ESP_OK)
                        CHARGELAB_LOG_MESSAGE(warning) << "Failed closing websocket client";
                }
            }

            settings_->saveIfModified();
            esp_restart();
        }

        bool isClockOutOfSync() override {
            return !detail::StaticPlatformESP::gSystemClockSet;
        }

        std::unique_ptr<chargelab::StorageInterface> getStorage(std::string const& file_name) override {
            auto path = std::string(kSpiffsPathPrefix) + "/" + file_name;
            return std::make_unique<chargelab::StorageFile>(std::move(path));
        }

        std::unique_ptr<chargelab::FlashBlockInterface> getPartition(std::string const& label) override {
            auto const partition = esp_partition_find_first(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, label.c_str());
            if (partition == nullptr)
                return nullptr;

            return std::make_unique<detail::FlashPartition>(partition);
        }

        std::shared_ptr<RestConnectionInterface> restRequest(RestMethod method, std::string const& uri) override {
            return std::make_shared<detail::RestRequest> (method, uri);
        }

        std::shared_ptr<WebsocketInterface> ocppConnection() override {
            if (websocket_ == nullptr) {
                std::optional<std::string> subprotocol;
                string::SplitVisitor(settings_->NetworkConfigurationPriority.transitionCurrentValue(), ",", [&](std::string const& value) {
                    if (subprotocol.has_value())
                        return;

                    auto index = string::ToInteger(value);
                    if (!index.has_value())
                        return;

                    auto const& profile = settings_->NetworkConnectionProfiles.transitionCurrentValue(index.value());
                    if (!profile.has_value())
                        return;

                    switch (profile->ocppVersion) {
                        default:
                            CHARGELAB_LOG_MESSAGE(error) << "Unrecognized OCPP version in network profile: " << profile;
                            break;

                        case ocpp2_0::OCPPVersionEnumType::kOCPP16:
                            subprotocol = "ocpp1.6";
                            break;

                        case ocpp2_0::OCPPVersionEnumType::kOCPP20:
                            subprotocol = "ocpp2.0.1";
                            break;
                    }
                });

                return std::make_shared<detail::StubWebsocket>(optional::GetOrDefault(subprotocol, std::string("ocpp1.6")));
            } else {
                return std::dynamic_pointer_cast<WebsocketInterface>(websocket_);
            }
        }

        void runStep(ocpp1_6::OcppRemote&) override {
        }

        void runStep(ocpp2_0::OcppRemote &remote) override {
            if (!pending_security_events_.empty()) {
                if (remote.sendCall(pending_security_events_.front()).has_value())
                    pending_security_events_.erase(pending_security_events_.begin());
            }
        }

        void runUnconditionally() override {
            reportHeapUsage();
            reportDiskUsage();
            reportCertificateMetrics();

            if (!initialised_) {
                initializePlatform();
                initialised_ = true;
                return;
            }

            if (!started_)
                return;

            updateApIp();
            checkNetworkConnection();
            updateTimeFromNtpOnBoot();
            updateWebsocketConnection();
            updateSecurityEvents();
        }

        std::shared_ptr<Settings> getSettings() override {
            return settings_;
        }

        bool addCertificate(std::string const& certificate, ocpp2_0::GetCertificateIdUseEnumType certificateType) {
            auto const hash_data = getCertificateHashData(certificate, certificateType);
            if (!hash_data.has_value())
                return false;

            std::optional<std::string> subjectNameHash;
            {
                //mbedtls_x509_time_is_future
                certs::MbedTlsCertificateResource holder;

                // Note: buflen is intended to include the null byte at the end of the string:
                // > buflen – The size of buf, including the terminating NULL byte in case of PEM encoded data.
                auto ret = mbedtls_x509_crt_parse(&holder.cert, (unsigned char const*)certificate.c_str(), certificate.size()+1);
                if (ret != 0) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Failed parsing certificate (ret=" << ret << "): " << certificate;
                    return false;
                }

                auto md_info = mbedtls_md_info_from_type(detail::StaticPlatformESP::kHashType);
                subjectNameHash = certs::getMessageDigestHex(md_info, holder.cert.subject_raw.p, holder.cert.subject_raw.len);
                if (!subjectNameHash.has_value()) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Failed calculating subject name hash for: " << certificate;
                    return false;
                }
            }

            addCertificate(detail::SavedCertificate {
                    certificateType,
                    hash_data.value(),
                    subjectNameHash.value(),
                    certificate
            });
            return true;
        }

        void addCertificate(detail::SavedCertificate const& certificate) {
            std::vector<detail::SavedCertificate> saved;
            auto state = certs::visitCustomCertificates([&](detail::SavedCertificate const& x) {
                saved.push_back(x);
            });
            saved.push_back(certificate);

            getStorage(kCertificatesFile)->write([&](auto file) {
                using settings = detail::StaticPlatformESP;
                std::fwrite(&settings::kCertificatesFileVersion, sizeof(settings::kCertificatesFileVersion), 1, file);

                int write_counter = 1;
                if (state.has_value())
                    write_counter = state->second + 1;

                std::fwrite(&write_counter, sizeof(write_counter), 1, file);

                for (auto const& x : saved)
                    file::json_write_object_to_file(file, x);

                return true;
            });
        }

        template <typename UnaryPred>
        int removeCertificatesIf(UnaryPred&& p) {
            int removed = 0;
            std::vector<detail::SavedCertificate> saved;
            auto state = certs::visitCustomCertificates([&](detail::SavedCertificate const& x) {
                if (p(x)) {
                    CHARGELAB_LOG_MESSAGE(info) << "Removing certificate: " << x;
                    removed++;
                } else {
                    saved.push_back(x);
                }
            });

            if (removed > 0) {
                getStorage(kCertificatesFile)->write([&](auto file) {
                    using settings = detail::StaticPlatformESP;
                    std::fwrite(&settings::kCertificatesFileVersion, sizeof(settings::kCertificatesFileVersion), 1, file);

                    int write_counter = 1;
                    if (state.has_value())
                        write_counter = state->second + 1;

                    std::fwrite(&write_counter, sizeof(write_counter), 1, file);

                    for (auto const& x : saved)
                        file::json_write_object_to_file(file, x);

                    return true;
                });
            }

            return removed;
        }

        bool verifyManufacturerCertificate(
                std::string const& pem,
                std::optional<SignatureAndHash> const& check_sha256
        ) override {
            certs::MbedTlsCertificateResource cert_holder;

            {
                // Note: buflen is intended to include the null byte at the end of the string:
                // > buflen – The size of buf, including the terminating NULL byte in case of PEM encoded data.
                auto ret = mbedtls_x509_crt_parse(&cert_holder.cert, (unsigned char const*)pem.c_str(), pem.size()+1);
                if (ret != 0) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Failed parsing certificate (ret=" << ret << "): " << pem;
                    return false;
                }
            }

            if (check_sha256.has_value())
            {
                auto ret = mbedtls_pk_verify(
                        &cert_holder.cert.pk,
                        MBEDTLS_MD_SHA256,
                        check_sha256->hash,
                        check_sha256->hash_len,
                        check_sha256->sig,
                        check_sha256->sig_len
                );
                if (ret != 0) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Signature verification failed (ret=" << ret << ")";
                    return false;
                }
            }

            auto md_info = mbedtls_md_info_from_type(detail::StaticPlatformESP::kHashType);
            auto raw_hash = certs::getMessageDigestHex(md_info, cert_holder.cert.issuer_raw.p, cert_holder.cert.issuer_raw.len);
            if (!raw_hash.has_value()) {
                CHARGELAB_LOG_MESSAGE(error) << "Failed calculating certificate hash";
                return false;
            }

            auto const issuerNameHash = ocpp2_0::IdentifierStringPrimitive<128> {raw_hash.value()};
            bool verified = false;
            certs::visitCustomCertificates([&](detail::SavedCertificate const& x) {
                if (verified)
                    return;
                if (x.certificateType != ocpp2_0::GetCertificateIdUseEnumType::kManufacturerRootCertificate)
                    return;
                if (x.subjectNameHash != issuerNameHash) {
                    CHARGELAB_LOG_MESSAGE(error) << "Skipping cert - subject name hash doesn't match";
                    return;
                }

                certs::MbedTlsCertificateResource ca_holder;

                // Note: buflen is intended to include the null byte at the end of the string:
                // > buflen – The size of buf, including the terminating NULL byte in case of PEM encoded data.
                auto ret = mbedtls_x509_crt_parse(&ca_holder.cert, (unsigned char const*)x.certificate.c_str(), x.certificate.size()+1);
                if (ret != 0) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Failed parsing certificate (ret=" << ret << "): " << x.certificate;
                    return;
                }

                std::string info;
                info.resize(1024, '\0');
                mbedtls_x509_crt_info(info.data(), info.size()-1, "", &ca_holder.cert);
                CHARGELAB_LOG_MESSAGE(info) << "Verifying manufacturer certificate against:\n" << info.c_str();

                std::optional<SystemTimeMillis> now;
                if (detail::StaticPlatformESP::gSystemClockSet)
                    now = detail::systemClockNow();

                if (certs::verifyCertificate(&cert_holder.cert, &ca_holder.cert, now)) {
                    verified = true;
                } else {
                    CHARGELAB_LOG_MESSAGE(error) << "Failed verifying cert";
                }
            });

            return verified;
        }

        bool isWifiConnected() const {
          return connected_;
        }

    private:
        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::InstallCertificateResponse>>
        onInstallCertificateReq(const ocpp2_0::InstallCertificateRequest &request) override {
            // Note: M04.FR.07 is ambiguous - assuming *the charging station* is free to set the "hashAlgorithm, which
            // was used to install the certificate" here.

            std::optional<std::string> subjectNameHash;
            {
                //mbedtls_x509_time_is_future
                certs::MbedTlsCertificateResource holder;

                // Note: buflen is intended to include the null byte at the end of the string:
                // > buflen – The size of buf, including the terminating NULL byte in case of PEM encoded data.
                auto const& certificate = request.certificate.value();
                auto ret = mbedtls_x509_crt_parse(&holder.cert, (unsigned char const*)certificate.c_str(), certificate.size()+1);
                if (ret != 0) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Failed parsing certificate (ret=" << ret << "): " << certificate;
                    return ocpp2_0::InstallCertificateResponse {ocpp2_0::InstallCertificateStatusEnumType::kRejected};
                }

                auto const now = systemClockNow();

                // M05.FR.07
                auto const valid_from = certs::convertToTimestamp(holder.cert.valid_from);
                auto const valid_to = certs::convertToTimestamp(holder.cert.valid_to);
                CHARGELAB_LOG_MESSAGE(debug) << "Certificate valid from: " << ocpp2_0::DateTime(valid_from);
                CHARGELAB_LOG_MESSAGE(debug) << "Certificate valid to: " << ocpp2_0::DateTime(valid_to);
                if (valid_from > now || valid_to < now)
                    return ocpp2_0::InstallCertificateResponse {ocpp2_0::InstallCertificateStatusEnumType::kRejected};

                auto md_info = mbedtls_md_info_from_type(detail::StaticPlatformESP::kHashType);
                subjectNameHash = certs::getMessageDigestHex(md_info, holder.cert.subject_raw.p, holder.cert.subject_raw.len);
                if (!subjectNameHash.has_value()) {
                    return ocpp2_0::InstallCertificateResponse {ocpp2_0::InstallCertificateStatusEnumType::kRejected};
                }
            }

            ocpp2_0::GetCertificateIdUseEnumType certificateType;
            switch (request.certificateType) {
                case ocpp2_0::InstallCertificateUseEnumType::kValueNotFoundInEnum:
                    // TODO - error response/protocol error?
                    return ocpp2_0::InstallCertificateResponse {ocpp2_0::InstallCertificateStatusEnumType::kRejected};

                case ocpp2_0::InstallCertificateUseEnumType::kV2GRootCertificate:
                    certificateType = ocpp2_0::GetCertificateIdUseEnumType::kV2GRootCertificate;
                    break;

                case ocpp2_0::InstallCertificateUseEnumType::kMORootCertificate:
                    certificateType = ocpp2_0::GetCertificateIdUseEnumType::kMORootCertificate;
                    break;

                case ocpp2_0::InstallCertificateUseEnumType::kCSMSRootCertificate:
                    certificateType = ocpp2_0::GetCertificateIdUseEnumType::kCSMSRootCertificate;
                    break;

                case ocpp2_0::InstallCertificateUseEnumType::kManufacturerRootCertificate:
                    certificateType = ocpp2_0::GetCertificateIdUseEnumType::kManufacturerRootCertificate;
                    break;
            }

            std::optional<ocpp2_0::CertificateHashDataType> certificateHashData;
            switch (request.certificateType) {
                case ocpp2_0::InstallCertificateUseEnumType::kCSMSRootCertificate:
                    certificateHashData = getCertificateHashData(
                            request.certificate.value(),
                            ocpp2_0::GetCertificateIdUseEnumType::kCSMSRootCertificate
                    );
                    break;

                case ocpp2_0::InstallCertificateUseEnumType::kManufacturerRootCertificate:
                    certificateHashData = getCertificateHashData(
                            request.certificate.value(),
                            ocpp2_0::GetCertificateIdUseEnumType::kManufacturerRootCertificate
                    );
                    break;

                default:
                    return ocpp2_0::InstallCertificateResponse {ocpp2_0::InstallCertificateStatusEnumType::kRejected};
            }

            if (!certificateHashData.has_value()) {
                // Note: not clear if M05.FR.03 or M05.FR.07 applies here
                return ocpp2_0::InstallCertificateResponse {ocpp2_0::InstallCertificateStatusEnumType::kRejected};
            }

            bool found = false;
            certs::visitCustomCertificates([&](detail::SavedCertificate const& x) {
                if (x.certificateType != certificateType)
                    return;
                if (x.certificateHashData != certificateHashData.value())
                    return;

                found = true;
            });

            if (found) {
                // M05.FR.17
                CHARGELAB_LOG_MESSAGE(info) << "Certificate was already present";
            } else {
                double max_certificates = 10;
                auto const model = settings_->CertificateEntries.getModel20();
                if (model.has_value()) {
                    auto const limit = model->variable_characteristics.maxLimit;
                    if (limit.has_value())
                        max_certificates = limit.value();
                }

                int current_certificates = 0;
                certs::visitCustomCertificates([&](auto const&) {current_certificates++;});

                // M05.FR.06
                if (current_certificates+1 > max_certificates) {
                    return ocpp2_0::InstallCertificateResponse {ocpp2_0::InstallCertificateStatusEnumType::kRejected};
                }

                addCertificate(detail::SavedCertificate {
                        certificateType,
                        certificateHashData.value(),
                        subjectNameHash.value(),
                        request.certificate.value()
                });
            }

            reportCertificateMetrics(true);
            return ocpp2_0::InstallCertificateResponse {ocpp2_0::InstallCertificateStatusEnumType::kAccepted};
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::DeleteCertificateResponse>>
        onDeleteCertificateReq(const ocpp2_0::DeleteCertificateRequest &request) override {
            bool found = false;
            certs::visitCustomCertificates([&](detail::SavedCertificate const& cert) {
                if (cert.certificateHashData == request.certificateHashData)
                    found = true;
            });

            if (!found) {
                return ocpp2_0::DeleteCertificateResponse {ocpp2_0::DeleteCertificateStatusEnumType::kNotFound};
            }

            removeCertificatesIf([&](detail::SavedCertificate const& cert) {
               return  cert.certificateHashData == request.certificateHashData;
            });

            reportCertificateMetrics(true);
            return ocpp2_0::DeleteCertificateResponse {ocpp2_0::DeleteCertificateStatusEnumType::kAccepted};
        }

        std::optional<ocpp2_0::ResponseToRequest<ocpp2_0::GetInstalledCertificateIdsResponse>>
        onGetInstalledCertificateIdsReq(const ocpp2_0::GetInstalledCertificateIdsRequest &request) override {
            int total_entries = 0;
            certs::visitCustomCertificates([&](detail::SavedCertificate const& cert) {
                if (request.certificateType.has_value()) {
                    if (!set::contains(request.certificateType.value(), cert.certificateType))
                        return;
                }

                total_entries++;
            });

            if (total_entries <= 0) {
                // M03.FR.02
                return ocpp2_0::GetInstalledCertificateIdsResponse {ocpp2_0::GetInstalledCertificateStatusEnumType::kNotFound};
            }

            return ocpp2_0::GetInstalledCertificateIdsResponse {
                ocpp2_0::GetInstalledCertificateStatusEnumType::kAccepted,
                {[=](std::function<void(ocpp2_0::CertificateHashDataChainType const &)> const &visitor) {
                    certs::visitCustomCertificates([&](detail::SavedCertificate const& cert) {
                        if (request.certificateType.has_value()) {
                            if (!set::contains(request.certificateType.value(), cert.certificateType))
                                return;
                        }

                        visitor(ocpp2_0::CertificateHashDataChainType {
                            cert.certificateType,
                            cert.certificateHashData
                        });
                    });
                }}
            };
        }

    private:
        std::optional<ocpp2_0::CertificateHashDataType> getCertificateHashData(
                std::string const& certificate,
                ocpp2_0::GetCertificateIdUseEnumType allowed_ca_type
        ) {
            // RAII holder to deallocate resources
            certs::MbedTlsCertificateResource holder;

            // Note: buflen is intended to include the null byte at the end of the string:
            // > buflen – The size of buf, including the terminating NULL byte in case of PEM encoded data.
            auto ret = mbedtls_x509_crt_parse(&holder.cert, (unsigned char const*)certificate.c_str(), certificate.size()+1);
            if (ret != 0) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed parsing certificate (ret=" << ret << "): " << certificate;
                return std::nullopt;
            }

            if (holder.cert.next != nullptr) {
                CHARGELAB_LOG_MESSAGE(warning) << "Multiple certificates were present in payload: " << certificate;
                return std::nullopt;
            }

            ocpp2_0::CertificateHashDataType result;
            auto md_info = mbedtls_md_info_from_type(detail::StaticPlatformESP::kHashType);
            result.hashAlgorithm = certs::toHashAlgorithm(detail::StaticPlatformESP::kHashType).value();
            if (md_info == nullptr) {
                CHARGELAB_LOG_MESSAGE(error) << "Failed fetching SHA256 MD info";
                return std::nullopt;
            }

            std::optional<SystemTimeMillis> now;
            if (detail::StaticPlatformESP::gSystemClockSet)
                now = detail::systemClockNow();

            std::optional<std::string> issuerNameHash;
            std::optional<std::string> issuerKeyHash;
            if (certs::verifyCertificate(holder, holder, now)) {
                // Certificate is self-signed
                issuerNameHash = certs::getMessageDigestHex(md_info, holder.cert.subject_raw.p, holder.cert.subject_raw.len);
                issuerKeyHash = certs::getCertificateKeyHash(md_info, holder);
            } else {
                // Certificate is not self-signed - need to find issuer authority
                bool found = false;
                certs::visitCustomCertificates([&](detail::SavedCertificate const& x) {
                    if (x.certificateType != allowed_ca_type)
                        return;

                    // RAII holder to deallocate resources
                    certs::MbedTlsCertificateResource holder_ca;

                    // Note: buflen is intended to include the null byte at the end of the string:
                    // > buflen – The size of buf, including the terminating NULL byte in case of PEM encoded data.
                    auto ret = mbedtls_x509_crt_parse(&holder_ca.cert, (unsigned char const*)x.certificate.c_str(), x.certificate.size() + 1);
                    if (ret != 0) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Failed parsing certificate (ret=" << ret << "): " << x.certificate;
                        return;
                    }

                    if (certs::verifyCertificate(holder, holder_ca, now)) {
                        found = true;
                        issuerNameHash = certs::getMessageDigestHex(md_info, holder_ca.cert.subject_raw.p, holder_ca.cert.subject_raw.len);
                        issuerKeyHash = certs::getCertificateKeyHash(md_info, holder_ca);
                    }
                });

                if (!found) {
                    CHARGELAB_LOG_MESSAGE(warning) << "Signing certificate not found for: " << certificate;
                    return std::nullopt;
                }
            }
            if (!issuerNameHash.has_value()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed calculating issuer name hash for: " << certificate;
                return std::nullopt;
            }
            if (!issuerKeyHash.has_value()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed calculating issuer key hash for: " << certificate;
                return std::nullopt;
            }

            // Note: serial number must be stripped of leading zeros as per description in 2.6.
            auto serialNumber = string::ToHexString((unsigned char*)holder.cert.serial.p, holder.cert.serial.len, "");
            serialNumber.erase(0, std::min(serialNumber.find_first_not_of('0'), serialNumber.size()-1));

            result.issuerNameHash = issuerNameHash.value();
            result.issuerKeyHash = issuerKeyHash.value();
            result.serialNumber = serialNumber;
            return result;
        }

    private:
        void initializePlatform() {
            CHARGELAB_LOG_MESSAGE(info) << "Initializing platform...";
            ESP_ERROR_CHECK(esp_netif_init());
            ESP_ERROR_CHECK(esp_event_loop_create_default());

            st_netif_ = esp_netif_create_default_wifi_sta();
            ap_netif_ = esp_netif_create_default_wifi_ap();
            assert(st_netif_);

            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            ESP_ERROR_CHECK(esp_wifi_init(&cfg));

            auto const client_ssid = settings_->WifiSSID.transitionCurrentValue();
            auto const client_password = settings_->WifiPassword.transitionCurrentValue();

            ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &EventHandler, this, nullptr));
            ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &EventHandler, this, nullptr));
            //ESP_ERROR_CHECK(esp_wifi_set_mode(client_ssid.empty() ? WIFI_MODE_AP : WIFI_MODE_APSTA));
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

            ESP_ERROR_CHECK(esp_wifi_start());

            if (!client_ssid.empty()) {
                wifi_config_t station_config {};
                strlcpy((char *) station_config.sta.ssid, client_ssid.c_str(), sizeof(station_config.sta.ssid));
                strlcpy((char *) station_config.sta.password, client_password.c_str(), sizeof(station_config.sta.password));

                station_config.sta.channel = 0;
                station_config.sta.pmf_cfg.capable = true;
                station_config.sta.pmf_cfg.required = false;

                ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &station_config));
                wifi_sta_enabled_ = true;
            }

            auto const ap_ssid = settings_->ChargerAccessPointSSID.getValue();
            auto const ap_password = settings_->WifiPassword.transitionCurrentValue();

            wifi_config_t ap_config {};
            strlcpy((char *) ap_config.ap.ssid, ap_ssid.c_str(), sizeof(ap_config.ap.ssid));
            strlcpy((char *) ap_config.sta.password, ap_password.c_str(), sizeof(ap_config.sta.password));
            ap_config.ap.authmode = ap_password.empty() ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
            ap_config.ap.ssid_hidden = !client_ssid.empty() && settings_->WifiHideStationAccessPointAfterSetup.getValue() ? 1 : 0;

            // TODO: Check what happens if this is exceeded; does the old connection get dropped or does the new
            //  connection get refused?
            ap_config.ap.max_connection = 1;

            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
        }

        void checkNetworkConnection() {
            if (!wifi_sta_enabled_)
                return;
            if (connected_)
                return;

            auto const now = steadyClockNow();
            if (last_connection_attempt_.has_value()) {
                auto const delta = now - last_connection_attempt_.value();
                if (delta / 1000 < settings_->WifiReconnectInterval.getValue())
                    return;
            }

            // Note: this block will only execute if valid wifi credentials were present on initialization due to the
            //       wifi_sta_enabled_ precondition above.
            CHARGELAB_LOG_MESSAGE(info) << "Retrying network connection";
            last_connection_attempt_ = now;
            auto connect_result = esp_wifi_connect();
            if (connect_result != ESP_OK) {
                CHARGELAB_LOG_MESSAGE(warning) << "wifi connection failed, error code:" << connect_result;
                last_connection_attempt_ = now;
            }
        }

        void updateTimeFromNtpOnBoot() {
            if (!connected_)
                return;
            if (polled_npt_server_)
                return;

            CHARGELAB_LOG_MESSAGE(info) << "Fetching initial system time from NTP servers";

            // TODO: This needs to be moved into configuration and possibly supported as a standard synchronization
            //  mechanism
            esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
            esp_netif_sntp_init(&config);
            if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK) {
                CHARGELAB_LOG_MESSAGE(warning) << "Polling NTP server timed out";
            } else {
                CHARGELAB_LOG_MESSAGE(info) << "System time set via NTP servers to: " << ocpp2_0::DateTime{systemClockNow()};
                detail::StaticPlatformESP::gSystemClockSet = true;
            }

            esp_netif_sntp_deinit();
            polled_npt_server_ = true;
        }

        void updateWebsocketConnection() {
            if (!connected_ || disable_websocket_connection_) {
                if (websocket_ != nullptr) {
                    CHARGELAB_LOG_MESSAGE(info) << "Stopping websocket client - wifi disconnected";
                    websocket_ = nullptr;
                }

                return;
            }

            if (websocket_ == nullptr) {
                int skipped = 0;
                std::optional<ocpp2_0::NetworkConnectionProfileType> profile;
                string::SplitVisitor(settings_->NetworkConfigurationPriority.transitionCurrentValue(), ",", [&](std::string const& value) {
                    if (profile.has_value())
                        return;

                    auto const index = string::ToInteger(value);
                    if (!index.has_value())
                        return;

                    auto const it = settings_->NetworkConnectionProfiles.transitionCurrentValue(index.value());
                    if (!it.has_value())
                        return;
                    if (skipped < network_profile_offset_) {
                        skipped++;
                        return;
                    }

                    profile = it.value();
                    settings_->ActiveNetworkProfile.setValue(index.value());
                });

                if (!profile.has_value()) {
                    if (settings_->hasPendingTransition(SettingTransitionType::kConnection)) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Failed to reconnect after connection setting update - rolling back transition";
                        settings_->rollback(SettingTransitionType::kConnection);
                        settings_->saveIfModified();
                        resetHard();
                    }

                    network_profile_offset_ = 0;
                    return;
                }

                std::string protocol;
                switch (profile->ocppVersion) {
                    default:
                    case ocpp2_0::OCPPVersionEnumType::kOCPP12:
                    case ocpp2_0::OCPPVersionEnumType::kOCPP15:
                        CHARGELAB_LOG_MESSAGE(warning) << "Skipping network profile - bad or not supported OCPP version: " << profile->ocppVersion;
                        return;

                    case ocpp2_0::OCPPVersionEnumType::kOCPP16:
                        protocol = "ocpp1.6";
                        break;

                    case ocpp2_0::OCPPVersionEnumType::kOCPP20:
                        protocol = "ocpp2.0.1";
                        break;
                }

                if (!next_connection_attempt_.has_value() || steadyClockNow() > next_connection_attempt_.value()) {
                    CHARGELAB_LOG_MESSAGE(info) << "Connecting using profile: " << profile;

                    auto const chargePointId = settings_->ChargePointId.transitionCurrentValue();
                    if (profile->ocppCsmsUrl.value().empty() || chargePointId.empty()) {
                        CHARGELAB_LOG_MESSAGE(warning) << "Empty csms url or charge point id";
                        return;
                    }
                    CHARGELAB_LOG_MESSAGE(info) << "Starting websocket client";
                    next_connection_attempt_ = std::nullopt;
                    settings_->DefaultMessageTimeout.setValue(profile->messageTimeout);
                    websocket_ = std::make_shared<detail::WebsocketImpl>(
                            settings_,
                            profile->ocppCsmsUrl.value(),
                            chargePointId,
                            protocol,
                            settings_->BasicAuthPassword.transitionCurrentValue(),
                            failed_connection_attempts_
                    );
                }
            } else {
                // TODO: fallback requirements
                // B10.FR.07

                if (failed_connection_attempts_ > settings_->NetworkProfileConnectionAttempts.getValue()) {
                    CHARGELAB_LOG_MESSAGE(info) << "Restarting websocket client - connection attempts exceeded: "
                        << (int)failed_connection_attempts_ << " >= "
                        << settings_->NetworkProfileConnectionAttempts.getValue();

                    network_profile_offset_++;
                    websocket_ = nullptr;
                    failed_connection_attempts_ = 0;
                } else if (!websocket_->running_) {
                    if (websocket_->pendingMessages() > 0) {
                        CHARGELAB_LOG_MESSAGE(info) << "Delaying connection cleanup - pending messages: " << websocket_->pendingMessages();
                    } else {
                        if (!websocket_->established_connection_) {
                            failed_connection_attempts_++;
                        } else {
                            failed_connection_attempts_ = 1;
                        }

                        auto const delay_seconds = detail::getDelaySeconds(*settings_, failed_connection_attempts_);
                        CHARGELAB_LOG_MESSAGE(info) << "Restarting websocket client - client thread was closed, delaying reconnection by: " << delay_seconds << " seconds";
                        websocket_ = nullptr;
                        next_connection_attempt_ = static_cast<SteadyPointMillis> (steadyClockNow() + delay_seconds*1000);
                    }
                }
            }
        }

        void updateSecurityEvents() {
            if (!detail::StaticPlatformESP::gInvalidCsmsCertificate)
                return;
            if (pending_security_events_.size() > kMaxPendingSecurityEvents)
                return;

            // TODO: Include reason? Likely makes more sense after refactoring to get rid of globals.
            pending_security_events_.push_back(ocpp2_0::SecurityEventNotificationRequest {
                    "InvalidCsmsCertificate",
                    systemClockNow()
            });
            detail::StaticPlatformESP::gInvalidCsmsCertificate = false;
        }

        void updateApIp() {
            std::string expected_ap_ip = kAccessPointIpAddress1;
            if (connected_) {
                auto current_st_ip = getStIpAddress();
                if (current_st_ip && current_st_ip.value().find(kAccessPointIpPrefix1) == 0) {
                    // Conflict between the AP and STA addresses - use secondary AP IP
                    expected_ap_ip = kAccessPointIpAddress2;
                }
            }

            auto const current_ap_ip = getAPIpAddress();
            if (current_ap_ip.has_value() && current_ap_ip.value() != expected_ap_ip) {
                CHARGELAB_LOG_MESSAGE(info) << "Changing internal AP address to: " << expected_ap_ip;
                setAPIpAddress(expected_ap_ip);
            }
        }

        void reportWiFiStrength() {
            auto const now = steadyClockNow();
            if (last_reported_wifi_) {
                auto const delta = now - last_reported_wifi_.value();
                if (delta/1000 < kWiFiStrengthReportFrequencySeconds)
                    return;
            }

            last_reported_wifi_ = now;

            wifi_ap_record_t ap;
            esp_wifi_sta_get_ap_info(&ap);

            settings_->DiagnosticWiFiSignalStrength.setValue(ap.rssi);
            CHARGELAB_LOG_MESSAGE(info) << "rssi: " << ap.rssi;
        }

    public:
        void reportHeapUsage() {
            using globals = chargelab::detail::ReportHeapGlobals;

            auto const now = steadyClockNow();
            if (last_reported_heap_.has_value()) {
                auto const delta = now - last_reported_heap_.value();
                if (delta / 1000 < kHeapReportFrequencySeconds)
                    return;
            }

            last_reported_heap_ = now;
            int free_heap = esp_get_free_heap_size();
            int free_heap_minimum = esp_get_minimum_free_heap_size();
            int free_heap_max_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

            int free_heap_minimum_since_last_report = globals::gMinFreeHeap;
            int free_heap_maximum_since_last_report = globals::gMaxFreeHeap;
            globals::gMinFreeHeap = -1;
            globals::gMaxFreeHeap = -1;

            settings_->DiagnosticFreeHeap.setValue(free_heap);
            settings_->DiagnosticFreeHeapMinimum.setValue(free_heap_minimum);

            CHARGELAB_LOG_MESSAGE(info) << "Free heap: " << free_heap
                                        << " Minimum free heap: " << free_heap_minimum
                                        << " Free max block heap: " << free_heap_max_block
                                        << " Maximum free heap since last report: " << free_heap_maximum_since_last_report
                                        << " Minimum free heap since last report: " << free_heap_minimum_since_last_report
                                        << " (" << free_heap - free_heap_minimum_since_last_report << ")";
        }

        // For testing purpose only
        void setDisableWebsocketConnection(bool disable_websocket_connection) {
            disable_websocket_connection_ = disable_websocket_connection;
        }
    private:
        void reportDiskUsage() {
            auto const now = steadyClockNow();
            if (last_reported_disk_.has_value()) {
                auto const delta = now - last_reported_disk_.value();
                if (delta/1000 < kDiskReportFrequencySeconds)
                    return;
            }

            last_reported_disk_ = now;

            size_t total = 0, used = 0;
            auto ret = esp_spiffs_info(spiffs_conf_.partition_label, &total, &used);
            if (ret != ESP_OK) {
                CHARGELAB_LOG_MESSAGE(error) << "Failed to get SPIFFS partition information: " << esp_err_to_name(ret);
                return;
            }

            CHARGELAB_LOG_MESSAGE(info) << "Disk usage: total " << total << ", used " << used << ", free " << total-used;
        }

        void reportCertificateMetrics(bool force = false) {
            auto const now = steadyClockNow();
            if (!force && last_reported_certificates_.has_value()) {
                auto const delta = now - last_reported_certificates_.value();
                if (delta/1000 < kCertificateReportFrequency)
                    return;
            }

            std::map<ocpp2_0::GetCertificateIdUseEnumType, int> counts;
            certs::visitCustomCertificates([&](detail::SavedCertificate const& x) {
                counts[x.certificateType]++;
            });

            settings_->InstalledV2GRootCertificateCount.setValue(counts[ocpp2_0::GetCertificateIdUseEnumType::kV2GRootCertificate]);
            settings_->InstalledMORootCertificateCount.setValue(counts[ocpp2_0::GetCertificateIdUseEnumType::kMORootCertificate]);
            settings_->InstalledCSMSRootCertificateCount.setValue(counts[ocpp2_0::GetCertificateIdUseEnumType::kCSMSRootCertificate]);
            settings_->InstalledV2GCertificateChainCount.setValue(counts[ocpp2_0::GetCertificateIdUseEnumType::kV2GCertificateChain]);
            settings_->InstalledManufacturerRootCertificateCount.setValue(counts[ocpp2_0::GetCertificateIdUseEnumType::kManufacturerRootCertificate]);
            last_reported_certificates_ = now;
        }

        static void EventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
        {
            auto this_ptr = static_cast<PlatformESP*> (arg);

            CHARGELAB_LOG_MESSAGE(info) << "wifi event:" << event_base <<", event id:" << event_id;

            if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
                CHARGELAB_LOG_MESSAGE(debug) << "Finished starting wifi station interface";
                this_ptr->started_ = true;
            } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
                CHARGELAB_LOG_MESSAGE(info) << "Disconnected from wifi network";
                this_ptr->connected_ = false;
            } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
                CHARGELAB_LOG_MESSAGE(info) << "Connected to wifi network";
                this_ptr->connected_ = true;
            }
        }

        std::optional<std::string> getAPIpAddress() {
            if (ap_netif_ == nullptr) return std::nullopt;

            esp_netif_ip_info_t ip_info;
            auto ret = esp_netif_get_ip_info(ap_netif_, &ip_info);
            if (ret != ESP_OK) {
                CHARGELAB_LOG_MESSAGE(error) << "failed to get ap Ip address, error code:" << ret;
                return std::nullopt;
            }
            char buf[16]{};
            if (esp_ip4addr_ntoa(&ip_info.ip, buf, sizeof(buf)) != nullptr) {
                CHARGELAB_LOG_MESSAGE(trace) << "got ap ip address:" << buf;
                return buf;
            } else {
                CHARGELAB_LOG_MESSAGE(error) << "failed to convert ap ip to string, ip:" << ip_info.ip.addr;
                return std::nullopt;
            }
        }

        bool setAPIpAddress(std::string const& ap_ip_address) {
            if (ap_netif_ == nullptr) return false;

            esp_netif_ip_info_t ip_info {
                    esp_ip4addr_aton(ap_ip_address.c_str()),
                    esp_ip4addr_aton("255.255.255.0"),
                    esp_ip4addr_aton(ap_ip_address.c_str())
            };

            bool ret = true;

            auto esp_ret = esp_netif_dhcps_stop(ap_netif_);
            if (esp_ret != ESP_OK) {
                CHARGELAB_LOG_MESSAGE(warning) << "failed to stop dhcp, error code:" << esp_ret;
            }

            esp_ret = esp_netif_set_ip_info(ap_netif_, &ip_info);

            if (esp_ret != ESP_OK) {
                CHARGELAB_LOG_MESSAGE(error) << "failed to set ap Ip address:" << ap_ip_address << ", error code:" << esp_ret;
                ret = false;
            } else {
                CHARGELAB_LOG_MESSAGE(trace) << "setting ap Ip address successfully, ip:" << ap_ip_address;
            }

            esp_ret = esp_netif_dhcps_start(ap_netif_);
            if (esp_ret != ESP_OK) {
                CHARGELAB_LOG_MESSAGE(error) << "failed to start dhcp, error code:" << esp_ret;
            }

            return ret;
        }

        std::optional<std::string> getStIpAddress() {
            if (st_netif_ == nullptr) return std::nullopt;

            esp_netif_ip_info_t ip_info;
            auto ret = esp_netif_get_ip_info(st_netif_, &ip_info);
            if (ret != ESP_OK) {
                CHARGELAB_LOG_MESSAGE(error) << "failed to get st Ip address, error code:" << ret;
                return std::nullopt;
            }
            char buf[16]{};
            if (esp_ip4addr_ntoa(&ip_info.ip, buf, sizeof(buf)) != nullptr) {
                return buf;
            } else {
                CHARGELAB_LOG_MESSAGE(error) << "failed to convert st ip to string, ip:" << ip_info.ip.addr;
                return std::nullopt;
            }
        }

    private:
        std::shared_ptr<Settings> settings_;
        std::shared_ptr<detail::WebsocketImpl> websocket_;

        bool initialised_ = false;
        bool polled_npt_server_ = false;

        std::atomic<bool> started_ = false;
        std::atomic<bool> connected_ = false;
        std::atomic<bool> wifi_sta_enabled_ = false;
        std::vector<ocpp2_0::SecurityEventNotificationRequest> pending_security_events_;

        std::optional<SteadyPointMillis> last_connection_attempt_ = std::nullopt;
        std::optional<SteadyPointMillis> last_reported_heap_ = std::nullopt;
        std::optional<SteadyPointMillis> last_reported_disk_ = std::nullopt;
        std::optional<SteadyPointMillis> last_reported_wifi_ = std::nullopt;
        std::optional<SteadyPointMillis> next_connection_attempt_ = std::nullopt;

        std::optional<SteadyPointMillis> last_reported_certificates_ = std::nullopt;

        // Note: not persisted
        // B10.FR.04
        int network_profile_offset_ = 0;
        std::atomic<int> failed_connection_attempts_ = 0;

        //
        esp_netif_t* ap_netif_{nullptr};
        esp_netif_t* st_netif_{nullptr};
        esp_vfs_spiffs_conf_t spiffs_conf_{};
        std::optional<std::string> current_ap_ip_ {std::nullopt};

        // For testing only, when it is true, the websocket will behave as not connected
        bool disable_websocket_connection_ = false;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_PLATFORM_ESP_H