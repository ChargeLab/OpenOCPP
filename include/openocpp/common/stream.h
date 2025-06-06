#ifndef CHARGELAB_OPEN_FIRMWARE_STREAM_H
#define CHARGELAB_OPEN_FIRMWARE_STREAM_H

#include <array>
#include <utility>

#include "openocpp/interface/element/byte_reader_interface.h"
#include "openocpp/interface/element/byte_writer_interface.h"

namespace chargelab::stream {
    class BufferedByteWriter {
    public:
        BufferedByteWriter(ByteWriterInterface& output)
            : output_(output)
        {
        }

        void put(char ch) {
            if (index_ >= buffer_.size())
                flush();

            buffer_[index_++] = ch;
        }

        void write(char const* s, std::size_t n) {
            for (int i = 0; i < n; i++) {
                put(s[i]);
            }
        }

        void write(char const* s) {
            write(s, std::strlen(s));
        }

        void write(std::string const &s) {
            write(s.c_str(), s.size());
        }

        void flush() {
            if (index_ == 0)
                return;

            output_.write(buffer_.data(), index_);
            index_ = 0;
        }

    private:
        ByteWriterInterface& output_;
        std::array<char, 128> buffer_ {};
        std::size_t index_ = 0;
    };

    class BufferedByteReader {
    public:
        BufferedByteReader(ByteReaderInterface& input)
            : input_(input)
        {
            next();
        }

        int peek() const {
            if (index_ >= count_)
                return EOF;

            return buffer_[index_];
        }

        int take() {
            if (index_ >= count_)
                return EOF;

            auto const result = buffer_[index_++];
            if (index_ >= count_ && count_ == buffer_.size())
                next();

            return result;
        }

        std::size_t tellg() const {
            return offset_ + index_;
        }

    private:
        void next() {
            count_ = input_.read(buffer_.data(), buffer_.size());
            index_ = 0;
        }

    private:
        ByteReaderInterface& input_;
        std::array<char, 128> buffer_ {};
        std::size_t index_ = 0;
        std::size_t count_ = 0;
        std::size_t offset_ = 0;
    };

    class StringReader : public ByteReaderInterface {
    public:
        explicit StringReader(std::string text) : text_(std::move(text))
        {
        }

        size_t read(char *s, std::size_t n) override {
            if (index_ >= text_.size())
                return 0;

            std::size_t count = std::min(n, text_.size() - index_);
            std::memcpy(s, &text_[index_], count);
            index_ += count;
            return count;
        }

    private:
        std::string text_;
        std::size_t index_ = 0;
    };

    class StringWriter : public ByteWriterInterface {
    public:
        void write(char const *s, std::size_t count) override {
            auto const index = text_.size();
            text_.resize(text_.size() + count);
            std::memcpy(&text_[index], s, count);
        }

        std::string const& str() const {
            return text_;
        }

    private:
        std::string text_;
    };

    class SizeCalculator : public ByteWriterInterface {
    public:
        [[nodiscard]] std::size_t getTotalBytes() const {
            return total_bytes_;
        }

    private:
        void write(const char *, std::size_t count) override {
            total_bytes_ += count;
        }

    private:
        std::size_t total_bytes_ = 0;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_STREAM_H
