#ifndef CHARGELAB_OPEN_FIRMWARE_COMPRESSED_QUEUE_H
#define CHARGELAB_OPEN_FIRMWARE_COMPRESSED_QUEUE_H

#include "openocpp/common/logging.h"

#include <optional>
#include <vector>
#include <cstdint>
#include <cstring>
#include <variant>
#include <string>

#include "zlib.h"

namespace chargelab {
    namespace detail {
        class CompressedStreamZlibConstants {
        public:
            // zlib constants
            static constexpr int kZlibLevel = Z_BEST_COMPRESSION;
            // 2KiB history buffer
            static constexpr int kZlibWindowBits = 11;
            static constexpr int kZlibMemLevel = 1;

            // Compressed block size threshold - when the size of the compressed records in a block exceeds this
            // threshold a new block is created to store subsequent records
            static constexpr std::size_t kCompressedBlockThreshold = 5 * 1024;

            static constexpr std::size_t kInflateBufferInitialSize = 256;
            static constexpr std::size_t kDeflateBufferStepSize = 256;
        };
    }

    class CompressedOutputStreamZLib {
    public:
        CompressedOutputStreamZLib(std::vector<uint8_t>& buffer, int record_flush = Z_NO_FLUSH)
            : buffer_(buffer), record_flush_(record_flush)
        {
            resetInternal();
        }

        ~CompressedOutputStreamZLib() {
            if (initialized_)
                deflateEnd(&stream_);
        }

        CompressedOutputStreamZLib(CompressedOutputStreamZLib const&) = delete;
        CompressedOutputStreamZLib(CompressedOutputStreamZLib&&) = delete;
        CompressedOutputStreamZLib& operator=(CompressedOutputStreamZLib const&) = delete;
        CompressedOutputStreamZLib& operator=(CompressedOutputStreamZLib&&) = delete;

        void addRecord(std::string_view const& record) {
            if (!initialized_)
                return;

            int size_prefix = (int)record.size();
            writeBlock(&size_prefix, sizeof(size_prefix), Z_NO_FLUSH);
            writeBlock(record.data(), record.size(), record_flush_);
        }

        void reset() {
            resetInternal();
        }

        bool close(bool finish_stream = true) {
            if (!initialized_)
                return false;

            if (finish_stream) {
                while(true) {
                    if (stream_.avail_out <= 0) {
                        auto const index = buffer_.size();
                        buffer_.resize(buffer_.size() + detail::CompressedStreamZlibConstants::kDeflateBufferStepSize);
                        stream_.next_out = buffer_.data() + index;
                        stream_.avail_out = buffer_.size() - index;
                    }

                    auto const ret = deflate(&stream_, Z_FINISH);
                    if (ret == Z_STREAM_END)
                        break;
                    if (ret != Z_OK) {
                        CHARGELAB_LOG_MESSAGE(warning) << "deflate failed with error code: " << ret;
                        return false;
                    }
                }
            }

            buffer_.resize(buffer_.size() - stream_.avail_out);
            deflateEnd(&stream_);
            std::memset(&stream_, 0, sizeof(stream_));
            initialized_ = false;
            return true;
        }

        [[nodiscard]] std::size_t approximateTotalBytes() const {
            if (!initialized_)
                return 0;

            return buffer_.size() - stream_.avail_out;
        }

    private:
        void writeBlock(void const* data, std::size_t size, int flush) {
            stream_.next_in = (uint8_t*)data;
            stream_.avail_in = size;

            do {
                if (stream_.avail_out <= 0) {
                    auto const index = buffer_.size() - stream_.avail_out;
                    buffer_.resize(buffer_.size() + detail::CompressedStreamZlibConstants::kDeflateBufferStepSize);
                    stream_.next_out = buffer_.data() + index;
                    stream_.avail_out = buffer_.size() - index;
                }

                auto const ret = deflate(&stream_, flush);
                if (ret != Z_OK) {
                    CHARGELAB_LOG_MESSAGE(warning) << "deflate failed with error code: " << ret;
                    return;
                }
            } while(stream_.avail_out == 0);

            stream_.next_in = Z_NULL;
            stream_.avail_in = 0;
        }

        void resetInternal() {
            buffer_.resize(detail::CompressedStreamZlibConstants::kDeflateBufferStepSize);
            stream_.zalloc = Z_NULL;
            stream_.zfree = Z_NULL;
            stream_.opaque = Z_NULL;
            stream_.next_in = Z_NULL;
            stream_.avail_in = 0;
            stream_.next_out = buffer_.data();
            stream_.avail_out = buffer_.size();
            auto const ret = deflateInit2(
                    &stream_,
                    detail::CompressedStreamZlibConstants::kZlibLevel,
                    Z_DEFLATED,
                    detail::CompressedStreamZlibConstants::kZlibWindowBits,
                    detail::CompressedStreamZlibConstants::kZlibMemLevel,
                    Z_DEFAULT_STRATEGY
            );
            if (ret == Z_OK) {
                initialized_ = true;
            } else {
                CHARGELAB_LOG_MESSAGE(warning) << "deflateInit failed with error code: " << ret;
            }
        }

    private:
        z_stream stream_ {};
        std::vector<uint8_t>& buffer_;
        int record_flush_;

        bool initialized_ = false;
    };

    class CompressedInputStreamZLib {
    public:
        CompressedInputStreamZLib(
                std::vector<uint8_t>& buffer,
                uint8_t* data,
                std::size_t size,
                bool ignore_read_errors = false
        )
            : buffer_(buffer),
              ignore_read_errors_(ignore_read_errors)
        {
            buffer_.resize(detail::CompressedStreamZlibConstants::kInflateBufferInitialSize);
            stream_.zalloc = Z_NULL;
            stream_.zfree = Z_NULL;
            stream_.opaque = Z_NULL;
            stream_.next_in = data;
            stream_.avail_in = size;
            stream_.next_out = buffer_.data();
            stream_.avail_out = buffer_.size();
            auto const ret = inflateInit2(&stream_, detail::CompressedStreamZlibConstants::kZlibWindowBits);
            if (ret == Z_OK) {
                initialized_ = true;
            } else {
                CHARGELAB_LOG_MESSAGE(warning) << "inflateInit failed with error code: " << ret;
            }
        }

        ~CompressedInputStreamZLib() {
            if (initialized_)
                inflateEnd(&stream_);
        }

        CompressedInputStreamZLib(CompressedInputStreamZLib const&) = delete;
        CompressedInputStreamZLib(CompressedInputStreamZLib&&) = delete;
        CompressedInputStreamZLib& operator=(CompressedInputStreamZLib const&) = delete;
        CompressedInputStreamZLib& operator=(CompressedInputStreamZLib&&) = delete;

        std::optional<std::string_view> nextRecord() {
            if (!initialized_)
                return std::nullopt;

            int size_value {};
            auto size_ptr = readBlock(sizeof(size_value));
            if (size_ptr == nullptr)
                return std::nullopt;

            std::memcpy(&size_value, size_ptr, sizeof(size_value));
            auto data_ptr = readBlock(size_value);
            if (data_ptr == nullptr) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed reading data block of size: " << size_value;
                return std::nullopt;
            }

            return std::string_view {(char const*)data_ptr, (std::size_t)size_value};
        }

        bool close() {
            if (!initialized_)
                return false;

            inflateEnd(&stream_);
            initialized_ = false;
            return true;
        }

    private:
        void const* readBlock(std::size_t size) {
            auto bytes_present = buffer_.size() - stream_.avail_out - index_;
            if (bytes_present >= size) {
                auto result = &buffer_[index_];
                index_ += size;
                return result;
            }
            if (buffer_.size() < size)
                buffer_.resize(size);

            std::memmove(buffer_.data(), buffer_.data()+index_, bytes_present);
            stream_.next_out = buffer_.data() + bytes_present;
            stream_.avail_out = buffer_.size() - bytes_present;
            index_ = 0;

            while (true) {
                auto const avail_begin = stream_.avail_in;
                auto const ret = inflate(&stream_, Z_BLOCK);
                auto const avail_end = stream_.avail_in;
                if (ret != Z_OK && ret != Z_STREAM_END) {
                    if (!ignore_read_errors_) {
                        CHARGELAB_LOG_MESSAGE(warning) << "inflate failed with error code " << ret << ": "
                                                       << (char const *) stream_.msg;
                    }

                    return nullptr;
                }

                bytes_present = buffer_.size() - stream_.avail_out - index_;
                if (bytes_present >= size)
                    break;
                if (avail_begin == avail_end)
                    return nullptr;
            }

            index_ += size;
            return buffer_.data();
        }

    private:
        z_stream stream_ {};
        std::vector<uint8_t>& buffer_;
        bool ignore_read_errors_;

        bool initialized_ = false;
        std::size_t index_ = 0;
    };

    class CompressedQueueRawZLib {
    public:
        CompressedQueueRawZLib() {
        }

        // Use poll first and pop first instead, and just remove when the attempts time-out
        std::optional<std::string> pollFront() {
            if (blocks_.empty())
                return std::nullopt;

            CompressedInputStreamZLib input {inflate_buffer_, blocks_.front().data(), blocks_.front().size()};
            auto const record = input.nextRecord();
            if (!record.has_value()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Unexpected state - no records present in chunk";
                blocks_.erase(blocks_.begin());
                return std::nullopt;
            }

            return std::string {record.value()};
        }

        std::optional<std::string> popFront() {
            if (blocks_.empty())
                return std::nullopt;

            CompressedInputStreamZLib input {inflate_buffer_, blocks_.front().data(), blocks_.front().size()};
            auto const record = input.nextRecord();
            if (!record.has_value()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Unexpected state - no records present in chunk";
                blocks_.erase(blocks_.begin());
                return std::nullopt;
            }
            auto result = std::string {record.value()};

            bool empty = true;
            CompressedOutputStreamZLib output {deflate_buffer_};
            while (true) {
                auto const& x = input.nextRecord();
                if (!x.has_value())
                    break;

                output.addRecord(x.value());
                empty = false;
            }

            output.close();
            if (empty) {
                blocks_.erase(blocks_.begin());
            } else {
                blocks_.front() = deflate_buffer_;
            }

            return result;
        }

        void updateFront(std::string const& update) {
            if (blocks_.empty())
                return;

            CompressedInputStreamZLib input {inflate_buffer_, blocks_.front().data(), blocks_.front().size()};
            auto const record = input.nextRecord();
            if (!record.has_value()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Unexpected state - no records present in chunk";
                blocks_.erase(blocks_.begin());
                return;
            }

            CompressedOutputStreamZLib output {deflate_buffer_};
            output.addRecord(update);

            while (true) {
                auto const& x = input.nextRecord();
                if (!x.has_value())
                    break;

                output.addRecord(x.value());
            }

            if (!output.close()) {
                CHARGELAB_LOG_MESSAGE(warning) << "Failed closing output stream - offline messages were dropped";
                blocks_.erase(blocks_.begin());
            } else {
                blocks_.front() = deflate_buffer_;
            }
        }

        void pushBack(std::string const& value) {
            if (!blocks_.empty() && blocks_.back().size() < detail::CompressedStreamZlibConstants::kCompressedBlockThreshold) {
                // Try to add the record to the final block
                {
                    CompressedOutputStreamZLib writer{deflate_buffer_};
                    CompressedInputStreamZLib reader{inflate_buffer_, blocks_.back().data(), blocks_.back().size()};
                    while (true) {
                        auto record = reader.nextRecord();
                        if (!record.has_value())
                            break;

                        writer.addRecord(record.value());
                    }

                    if (!reader.close()) {
                        CHARGELAB_LOG_MESSAGE(error) << "Failed reading historic data - dropping record";
                        clear();
                        return;
                    }

                    writer.addRecord(value);
                    if (!writer.close()) {
                        CHARGELAB_LOG_MESSAGE(error) << "Failed closing compressed stream - dropping record" ;
                        clear();
                        return;
                    }
                }

                // Note: freeing the reader and writer before resizing buffer
                blocks_.back() = deflate_buffer_;
            } else {
                // Otherwise add a new block
                {
                    CompressedOutputStreamZLib writer{deflate_buffer_};
                    writer.addRecord(value);
                    if (!writer.close()) {
                        CHARGELAB_LOG_MESSAGE(error) << "Failed closing compressed stream - dropping record";
                        clear();
                        return;
                    }
                }

                // Note: freeing the writer before adding new buffer
                blocks_.push_back(deflate_buffer_);
            }
        }

        template <typename Visitor>
        void visit(Visitor&& visitor) {
            for (auto& block : blocks_) {
                CompressedInputStreamZLib reader{inflate_buffer_, block.data(), block.size()};
                while (true) {
                    auto const& record = reader.nextRecord();
                    if (!record.has_value())
                        break;

                    visitor(record.value());
                }
            }
        }

        template <typename Predicate>
        void removeIf(Predicate&& predicate) {
            bool empty = true;
            CompressedOutputStreamZLib writer{deflate_buffer_};

            std::vector<std::vector<uint8_t>> original_blocks;
            std::swap(blocks_, original_blocks);

            for (auto& block : original_blocks) {
                CompressedInputStreamZLib reader{inflate_buffer_, block.data(), block.size()};
                while (true) {
                    auto const& record = reader.nextRecord();
                    if (!record.has_value())
                        break;
                    if (predicate(record.value()))
                        continue;

                    empty = false;
                    writer.addRecord(record.value());

                    if (writer.approximateTotalBytes() > detail::CompressedStreamZlibConstants::kCompressedBlockThreshold) {
                        if (!writer.close()) {
                            CHARGELAB_LOG_MESSAGE(error) << "Failed closing compressed stream - blocks were dropped";
                        } else {
                            blocks_.push_back(deflate_buffer_);
                        }

                        empty = true;
                        writer.reset();
                    }
                }

                std::vector<uint8_t> empty_buffer{};
                std::swap(empty_buffer, block);

                // This may be a long-running operation, so sleep briefly between individual blocks to yield to other
                // threads
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(1ms);
            }

            if (!empty) {
                if (!writer.close()) {
                    CHARGELAB_LOG_MESSAGE(error) << "Failed closing compressed stream - blocks were dropped";
                } else {
                    blocks_.push_back(deflate_buffer_);
                }
            }
        }

        void clear() {
            blocks_.clear();
        }

        [[nodiscard]] std::size_t totalBytes() const {
            std::size_t result = 0;
            for (auto const& block : blocks_)
                result += block.size();

            return result;
        }

        [[nodiscard]] bool empty() const {
            return blocks_.empty();
        }

        template<typename Visitor>
        void write(Visitor&& visitor) {
            for (auto const& block : blocks_)
                visitor((void*)block.data(), block.size());
        }

        template<typename Supplier>
        void read(Supplier&& supplier) {
            blocks_.clear();
            while (true) {
                auto const next = supplier();
                if (!next.has_value())
                    break;

                blocks_.push_back(next.value());
            }
        }

    private:
        // Note: using a shared deflate and inflate buffer here to reduce memory fragmentation
        std::vector<std::vector<uint8_t>> blocks_;
        std::vector<uint8_t> deflate_buffer_;
        std::vector<uint8_t> inflate_buffer_;
    };

    template <typename T, typename Serializer>
    class CompressedQueueCustom {
    public:
        std::optional<T> pollFront() {
            auto text = queue_.pollFront();
            if (!text.has_value())
                return std::nullopt;

            return Serializer::read(text.value());
        }

        std::optional<T> popFront() {
            auto text = queue_.popFront();
            if (!text.has_value())
                return std::nullopt;

            return Serializer::read(text.value());
        }

        void updateFront(T const& update) {
            queue_.updateFront(Serializer::write(update));
        }

        void pushBack(T const& value) {
            queue_.pushBack(Serializer::write(value));
        }

        template<typename Visitor>
        void visit(Visitor&& visitor) {
            queue_.visit([&](auto const& text) {
                auto result = Serializer::read(text);
                if (result.has_value()) {
                    visitor(text, result.value());
                } else {
                    CHARGELAB_LOG_MESSAGE(warning) << "Failed deserializing payload: " << text;
                }
            });
        }

        template <typename Predicate>
        void removeIf(Predicate&& predicate) {
            queue_.template removeIf([&](auto const& text) {
                auto result = Serializer::read(text);
                if (result.has_value()) {
                    return predicate(text, result.value());
                } else {
                    CHARGELAB_LOG_MESSAGE(warning) << "Failed deserializing payload";
                    return true;
                }
            });
        }

        void clear() {
            queue_.clear();
        }

        [[nodiscard]] std::size_t totalBytes() const {
            return queue_.totalBytes();
        }

        [[nodiscard]] bool empty() const {
            return queue_.empty();
        }

        template<typename Visitor>
        void write(Visitor&& visitor) {
            queue_.template write(std::forward<Visitor>(visitor));
        }

        template<typename Supplier>
        void read(Supplier&& supplier) {
            queue_.template read(std::forward<Supplier>(supplier));
        }

    private:
        CompressedQueueRawZLib queue_;
    };

    namespace detail {
        //payload_to_string(request)
        template <typename T>
        struct JsonSerializer {
            static std::optional<T> read(std::string_view const& text) {
                return read_json_from_string<T>(text);
            }

            static std::string write(T const& value) {
                return write_json_to_string(value);
            }
        };
    }

    template <typename T>
    using CompressedQueueJson = CompressedQueueCustom<T, detail::JsonSerializer<T>>;
}

#endif //CHARGELAB_OPEN_FIRMWARE_COMPRESSED_QUEUE_H
