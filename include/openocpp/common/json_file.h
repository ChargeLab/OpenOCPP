#ifndef CHARGELAB_OPEN_FIRMWARE_JSON_FILE_H
#define CHARGELAB_OPEN_FIRMWARE_JSON_FILE_H
#include <optional>
#include <string>
#include "openocpp/helpers/json.h"
#include "logging.h"

namespace chargelab {
    class JsonFile {
        static constexpr int kMaxFileDumpSize = 10*1000;
    public:
        template<class T>
        static std::optional<T> loadFile(std::string const& file_name) {
            auto file_content = readTextFile(file_name.c_str());
            if (file_content) {
                return read_json_from_string<T>(file_content);
            }

            return std::nullopt;
        }

        template<class T>
        static bool saveProfiles(std::string const& file_name, T const & content) {
            FILE* file = fopen(file_name.c_str(), "w");
            if (file == nullptr) {
                CHARGELAB_LOG_MESSAGE(error) << "Failed to open file for writing: " << file_name;
                return false;
            }

            auto const json_string = write_json_to_string(content);
            if (!json_string.has_value()) {
                CHARGELAB_LOG_MESSAGE(error) << "Failed serializing JSON content";
                return false;
            }

            if (fwrite(json_string->data(), 1, json_string->size(), file) < json_string.size()) {
                fclose(file);
                CHARGELAB_LOG_MESSAGE(error) << "Failed to write file: " << file_name;
                return false;
            }

            fclose(file);
            return true;
        }

    private:
        static std::optional<std::string> readTextFile(const char* file_name) {
            FILE* file = fopen(file_name, "r");
            if (file == nullptr) {
                CHARGELAB_LOG_MESSAGE(debug) << "file doesn't exist:" << file_name;
                return std::nullopt;
            }

            fseek(file, 0, SEEK_END);
            size_t file_size = ftell(file);
            fseek(file, 0, SEEK_SET);

            std::string text;
            text.resize(file_size);
            if (fread(text.data(), 1, file_size, file) != file_size) {
                fclose(file);
                CHARGELAB_LOG_MESSAGE(error) << "Failed reading file:" << file_name << " file size:" << file_size;
                return std::nullopt;
            }
            fclose(file);

            return text;
        }
    };
}


#endif //CHARGELAB_OPEN_FIRMWARE_JSON_FILE_H
