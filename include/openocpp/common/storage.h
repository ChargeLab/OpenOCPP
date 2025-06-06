#ifndef CHARGELAB_OPEN_FIRMWARE_STORAGE_H
#define CHARGELAB_OPEN_FIRMWARE_STORAGE_H

#include <string>

#include "openocpp/interface/element/storage_interface.h"

namespace chargelab {
    namespace detail {
        class CloseFileWrapper {
        public:
            CloseFileWrapper(std::FILE* file)
                : file_(file)
            {
            }

            ~CloseFileWrapper() {
                std::fclose(file_);
            }

        private:
            std::FILE* file_;
        };
    }

    class StorageFile : public StorageInterface {
    public:
        explicit StorageFile(std::string filename) : filename_(std::move(filename))
        {
        }

    public:
        bool read(std::function<bool(FILE *)> const& function) override {
            auto file = std::fopen(filename_.c_str(), "r");
            if (file == nullptr)
                return false;

            detail::CloseFileWrapper wrapper{file};
            return function(file);
        }

        bool write(std::function<bool(FILE *)> const& function) override {
            auto file = std::fopen(filename_.c_str(), "w");
            if (file == nullptr)
                return false;

            detail::CloseFileWrapper wrapper{file};
            return function(file);
        }

    private:
        std::string filename_;
    };

    class StorageNull : public StorageInterface {
    public:
        bool read(std::function<bool(FILE *)> const&) override {
            return false;
        }

        bool write(std::function<bool(FILE *)> const&) override {
            return false;
        }
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_STORAGE_H
