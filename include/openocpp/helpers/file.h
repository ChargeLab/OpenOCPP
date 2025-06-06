#ifndef CHARGELAB_OPEN_FIRMWARE_FILE_H
#define CHARGELAB_OPEN_FIRMWARE_FILE_H

#include "openocpp/common/logging.h"
#include "openocpp/helpers/json.h"

#include <cstdio>
#include <cctype>

namespace chargelab::file {
    namespace detail {
        template<typename CharType=char>
        class FileByteWriter : public ByteWriterInterface {
        public:
            FileByteWriter(FILE* file) : file_(file) {
                assert(file_ != nullptr);
            }

            ~FileByteWriter() {
            }

            void write(const char *s, std::size_t length) override {
                std::fwrite(s, sizeof(CharType), length, file_);
            }

        private:
            FILE* file_;
            std::array<CharType, 128> buffer_ {};
            std::size_t index_ = 0;
        };
    }

    inline bool is_eof_ignore_whitespace(FILE* file) {
        while (true) {
            auto ch = std::fgetc(file);
            if (ch == EOF) {
                return true;
            }
            if (!std::isspace(ch)) {
                std::ungetc(ch, file);
                return false;
            }
        }
    }

    template <typename T>
    std::optional<T> json_read_object_from_file(FILE* file) {
        if (is_eof_ignore_whitespace(file))
            return std::nullopt;

        int ch;
        std::string line;
        while ((ch = std::fgetc(file)) != EOF) {
            if (ch == '\r' || ch == '\n')
                break;

            line += (char)ch;
        }

        auto const result = read_json_from_string<T>(line);
        if (!result.has_value()) {
            CHARGELAB_LOG_MESSAGE(warning) << "Failed parsing line as JSON: " << line;
            return std::nullopt;
        }

        return result;
    }

    template<typename T>
    void json_write_object_to_file(FILE* file, T&& value) {
        write_json(detail::FileByteWriter<char> {file}, value);
        std::fputc('\n', file);
    }
}

#endif //CHARGELAB_OPEN_FIRMWARE_FILE_H
