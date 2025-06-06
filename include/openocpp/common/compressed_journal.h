#ifndef CHARGELAB_OPEN_FIRMWARE_COMPRESSED_JOURNAL_H
#define CHARGELAB_OPEN_FIRMWARE_COMPRESSED_JOURNAL_H

#include "openocpp/common/compressed_queue.h"
#include "openocpp/interface/element/flash_block_interface.h"

namespace chargelab {
    template <typename T, typename Serializer>
    class CompressedJournalCustom {
    public:
        explicit CompressedJournalCustom(std::unique_ptr<FlashBlockInterface> storage, std::string protocol_version)
            : storage_(std::move(storage)),
              protocol_version_(std::move(protocol_version))
        {
        }

        bool addUpdate(std::vector<T> const& final_state, T const& delta) {
            // First try to add the delta to the stream
            if (addToStream(std::vector<T>{delta}))
                return true;

            // Otherwise erase flash storage and write the final state
            CHARGELAB_LOG_MESSAGE(info) << "Available journal storage exhausted - clearing";
            storage_->erase();
            writeHeader();
            return addToStream(final_state);
        }

        template <typename Visitor>
        void visit(Visitor&& visitor) {
            if (!checkHeader())
                return;

            std::vector<uint8_t> raw_data;
            raw_data.resize(storage_->size() - protocol_version_.size());
            storage_->read(protocol_version_.size(), raw_data.data(), raw_data.size());

            std::vector<uint8_t> input_buffer;
            CompressedInputStreamZLib input(input_buffer, raw_data.data(), raw_data.size(), true);
            while (true) {
                auto record = input.nextRecord();
                if (!record.has_value())
                    break;

                auto result = Serializer::read(record.value());
                if (result.has_value()) {
                    visitor(record.value(), result.value());
                } else {
                    CHARGELAB_LOG_MESSAGE(warning) << "Failed deserializing payload: " << record.value();
                }
            }
        }

        [[nodiscard]] std::size_t totalBytesWritten() const {
            return bytes_written_;
        }

        [[nodiscard]] std::size_t storageSize() const {
            return storage_->size();
        }

    private:
        bool checkHeader() {
            std::string test;
            test.resize(protocol_version_.size());
            storage_->read(0, test.data(), test.size());
            return test == protocol_version_;
        }

        bool writeHeader() {
            return storage_->write(0, protocol_version_.data(), protocol_version_.size());
        }

        bool addToStream(std::vector<T> const& records) {
            if (!checkHeader())
                return false;

            std::vector<uint8_t> raw_data;
            raw_data.resize(storage_->size() - protocol_version_.size());
            storage_->read(protocol_version_.size(), raw_data.data(), raw_data.size());

            std::vector<uint8_t> input_buffer;
            std::vector<uint8_t> output_buffer;
            CompressedInputStreamZLib input(input_buffer, raw_data.data(), raw_data.size(), true);
            CompressedOutputStreamZLib output(output_buffer, Z_SYNC_FLUSH);
            while (true) {
                auto record = input.nextRecord();
                if (!record.has_value())
                    break;

                output.addRecord(record.value());
            }

            for (auto const& x : records)
                output.addRecord(Serializer::write(x));

            if (!output.close(false) || output_buffer.size() > raw_data.size())
                return false;

            total_bytes_ = output_buffer.size();

            std::size_t first_index = 0;
            while (first_index < output_buffer.size()) {
                auto const old_byte = raw_data[first_index];
                auto const new_byte = output_buffer[first_index];

                // Note: assuming write operations can only set bits to zero in between erase operations
                if ((old_byte & new_byte) != new_byte)
                    return false;

                if (old_byte != new_byte)
                    break;

                first_index++;
            }

            if (first_index >= output_buffer.size())
                return true;

            auto last_index = first_index;
            for (auto i = first_index; i < output_buffer.size(); i++) {
                auto const old_byte = raw_data[i];
                auto const new_byte = output_buffer[i];

                // Note: assuming write operations can only set bits to zero in between erase operations
                if ((old_byte & new_byte) != new_byte)
                    return false;
                if (old_byte != new_byte)
                    last_index = i;
            }

            auto const size = last_index - first_index + 1;
            bytes_written_ += size;
            return storage_->write(protocol_version_.size() + first_index, &output_buffer[first_index], size);
        }

    private:
        std::unique_ptr<FlashBlockInterface> storage_;
        std::string protocol_version_;
        std::optional<std::size_t> total_bytes_;

        std::atomic<std::size_t> bytes_written_ = 0;
    };

    template <typename T>
    using CompressedJournalJson = CompressedJournalCustom<T, detail::JsonSerializer<T>>;
}

#endif //CHARGELAB_OPEN_FIRMWARE_COMPRESSED_JOURNAL_H
