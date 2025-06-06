#ifndef CHARGELAB_OPEN_FIRMWARE_STATION_ESP32_H
#define CHARGELAB_OPEN_FIRMWARE_STATION_ESP32_H

#include "openocpp/interface/station_interface.h"
#include "openocpp/common/logging.h"
#include "openocpp/helpers/string.h"

#include "esp_ota_ops.h"

namespace chargelab {
    class StationEsp32 : public StationInterface {
    public:
        StationEsp32()
                : running_partition_(esp_ota_get_running_partition()),
                  update_partition_(esp_ota_get_next_update_partition(nullptr))
        {
            assert(running_partition_ != nullptr);
            assert(update_partition_ != nullptr);
            ESP_ERROR_CHECK(esp_ota_mark_app_valid_cancel_rollback());

            CHARGELAB_LOG_MESSAGE(debug) << "Running partition: " << running_partition_->label;
            CHARGELAB_LOG_MESSAGE(debug) << "Update partition: " << update_partition_->label;
        }

        std::string getActiveSlotId() override {
            std::array<uint8_t, sizeof(uint32_t)> buffer {};
            std::memcpy(buffer.data(), &running_partition_->address, buffer.size());
            return string::ToHexString(buffer, "");
        }

        std::string getUpdateSlotId() override {
            std::array<uint8_t, sizeof(uint32_t)> buffer {};
            std::memcpy(buffer.data(), &update_partition_->address, buffer.size());
            return string::ToHexString(buffer, "");
        }

        Result startUpdateProcess(std::size_t update_size) override {
            if (update_handle_ != 0) {
                CHARGELAB_LOG_MESSAGE(error) << "Another firmware update operation was in progress";
                return Result::kFailed;
            }

            if (update_size > update_partition_->size) {
                CHARGELAB_LOG_MESSAGE(warning) << "Update size exceeded partition size: " << update_size << " > " << update_partition_->size;
                return Result::kFailed;
            }

            if (esp_err_t esp_status = esp_ota_begin(update_partition_, OTA_SIZE_UNKNOWN, &update_handle_) != ESP_OK) {
                CHARGELAB_LOG_MESSAGE(error) << "Failed to start firmware update - error " << esp_status;
                return Result::kFailed;
            }

            CHARGELAB_LOG_MESSAGE(info) << "Starting firmware update for partition: " << update_partition_->label;
            update_size_ = update_size;
            update_offset_ = 0;
            return Result::kSucceeded;
        }

        Result processFirmwareChunk(const std::uint8_t *block, std::size_t size) override {
            if (update_handle_ == 0) {
                CHARGELAB_LOG_MESSAGE(error) << "Process chunk called outside of a firmware update operation";
                return Result::kFailed;
            }

            if (update_offset_ + size > update_size_) {
                CHARGELAB_LOG_MESSAGE(error) << "Firmware update chunks exceeded update size";
                return Result::kFailed;
            }

            if (esp_err_t esp_status = esp_ota_write(update_handle_, (const void*)block, size) != ESP_OK) {
                CHARGELAB_LOG_MESSAGE(error) << "Firmware update write failed - error " << esp_status;
                esp_ota_abort(update_handle_);
                update_handle_ = 0;
                return Result::kFailed;
            }

            update_size_ += size;
            return Result::kSucceeded;
        }

        Result finishUpdateProcess(bool succeeded) override {
            if (update_handle_ == 0) {
                CHARGELAB_LOG_MESSAGE(error) << "Finish update called outside of a firmware update operation";
                return Result::kFailed;
            }

            if (succeeded) {
                esp_ota_end(update_handle_);
                esp_ota_set_boot_partition(update_partition_);
                CHARGELAB_LOG_MESSAGE(info) << "Update applied - the next boot will attempt to boot into the updated partition";
            } else {
                esp_ota_abort(update_handle_);
            }

            update_handle_ = 0;
            return Result::kSucceeded;
        }

    private:
        esp_partition_t const* running_partition_;
        esp_partition_t const* update_partition_;

        esp_ota_handle_t update_handle_ = 0;
        std::size_t update_size_ = 0;
        std::size_t update_offset_ = 0;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_STATION_ESP32_H
