#ifndef CHARGELAB_OPEN_FIRMWARE_HASH_METHODS_MBEDTLS_H
#define CHARGELAB_OPEN_FIRMWARE_HASH_METHODS_MBEDTLS_H

#include "openocpp/common/logging.h"
#include "openocpp/helpers/string.h"

#include <string>
#include <optional>

#include "mbedtls/ssl.h"
#include "mbedtls/base64.h"

namespace chargelab {
    class HashMethodsMbedTLS;

    namespace detail {
        class HashCalculatorTypeMbedTLS {
        private:
            friend class ::chargelab::HashMethodsMbedTLS;

            HashCalculatorTypeMbedTLS() {
                mbedtls_md_init(&context_);
            }

        public:
            ~HashCalculatorTypeMbedTLS() {
                mbedtls_md_free(&context_);
            }

            bool update(unsigned char const* input, std::size_t ilen) {
                int err = mbedtls_md_update(&context_, input, ilen);
                if (err != 0) {
                    CHARGELAB_LOG_MESSAGE(error) << "Update failed with error: " << err;
                    return false;
                }

                return true;
            }

            std::optional<std::vector<uint8_t>> finishBinary() {
                auto md_info = mbedtls_md_info_from_ctx(&context_);
                if (md_info == nullptr) {
                    CHARGELAB_LOG_MESSAGE(error) << "Unexpected state - missing md_info in context";
                    return std::nullopt;
                }

                std::vector<uint8_t> result;
                result.resize(mbedtls_md_get_size(md_info));

                int err = mbedtls_md_finish(&context_, (unsigned char*)result.data());
                if (err != 0) {
                    CHARGELAB_LOG_MESSAGE(error) << "Finish failed with error: " << err;
                    return std::nullopt;
                }

                err = mbedtls_md_starts(&context_);
                if (err != 0) {
                    CHARGELAB_LOG_MESSAGE(error) << "Unexpected state - Failed resetting MbedTLS MD context";
                }

                return result;
            }

            std::optional<std::string> finishHex() {
                auto binary = finishBinary();
                if (!binary.has_value())
                    return std::nullopt;

                return string::ToHexString((unsigned char*)binary->data(), binary->size(), "");
            }

            void reset() {
                finishBinary();
            }

        private:
            mbedtls_md_context_t context_;
        };
    }

    class HashMethodsMbedTLS {
    private:
        HashMethodsMbedTLS() {}

    public:
        using hash_enum_type = mbedtls_md_type_t;
        using hash_calculator_type = detail::HashCalculatorTypeMbedTLS;

        static constexpr hash_enum_type kHashTypeSHA256 = MBEDTLS_MD_SHA256;
        static constexpr hash_enum_type kHashTypeSHA512 = MBEDTLS_MD_SHA512;

        static std::unique_ptr<hash_calculator_type> createCalculator(hash_enum_type hash_type) {
            auto md_info = mbedtls_md_info_from_type(hash_type);
            if (md_info == nullptr) {
                CHARGELAB_LOG_MESSAGE(error) << "Missing message digest info";
                return nullptr;
            }

            auto result = std::unique_ptr<detail::HashCalculatorTypeMbedTLS> (new detail::HashCalculatorTypeMbedTLS());
            int err = mbedtls_md_setup(&result->context_, md_info, 0);
            if (err != 0) {
                CHARGELAB_LOG_MESSAGE(error) << "Failed setting up context - error code: " << err;
                return nullptr;
            }

            err = mbedtls_md_starts(&result->context_);
            if (err != 0) {
                CHARGELAB_LOG_MESSAGE(error) << "Failed starting context - error code: " << err;
                return nullptr;
            }

            return result;
        }

        static std::optional<std::vector<uint8_t>> calculateHashBinary(
                hash_enum_type hash_type,
                unsigned char const* input,
                std::size_t ilen
        ) {
            auto md_info = mbedtls_md_info_from_type(hash_type);
            if (md_info == nullptr) {
                CHARGELAB_LOG_MESSAGE(warning) << "Missing message digest info";
                return std::nullopt;
            }

            std::vector<uint8_t> digest_buffer;
            digest_buffer.resize(mbedtls_md_get_size(md_info));
            auto ret = mbedtls_md(md_info, input, ilen, (unsigned char*)digest_buffer.data());
            if (ret != 0) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed calculating message digest - error: " << ret;
                return std::nullopt;
            }

            return digest_buffer;
        }

        static std::optional<std::string> calculateHashHex(
                hash_enum_type hash_type,
                unsigned char const* input,
                std::size_t ilen
        ) {
            auto binary = calculateHashBinary(hash_type, input, ilen);
            if (!binary.has_value())
                return std::nullopt;

            return string::ToHexString((unsigned char*)binary->data(), binary->size(), "");
        }

        static std::optional<std::vector<uint8_t>> decodeBase64(std::string const& base64_text) {
            std::size_t olen = 0;
            auto ret = mbedtls_base64_decode(nullptr, 0, &olen, (const unsigned char*)base64_text.data(), base64_text.size());
            if (ret != 0 && ret != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed decoding base64 text/calculating size - error: " << ret;
                return std::nullopt;
            }

            std::vector<uint8_t> result;
            result.resize(olen);

            ret = mbedtls_base64_decode(result.data(), result.size(), &olen, (const unsigned char*)base64_text.data(), base64_text.size());
            if (ret != 0) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed decoding base64 text - error: " << ret;
                return std::nullopt;
            }

            return result;
        }
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_HASH_METHODS_MBEDTLS_H
