#ifndef CHARGELAB_OPEN_FIRMWARE_REST_CONNECTION_INTERFACE_H
#define CHARGELAB_OPEN_FIRMWARE_REST_CONNECTION_INTERFACE_H

#include <optional>

namespace chargelab {
    class RestConnectionInterface {
    public:
        virtual ~RestConnectionInterface() = default;

        virtual int getStatusCode() = 0;
        virtual std::size_t getContentLength() = 0;

        /**
         * Sets a custom header. Must be called before open().
         * @param key
         * @param value
         */
        virtual void setHeader(std::string const& key, std::string const& value) = 0;

        /**
         * Opens the connection to the server. Must be called after setting any headers for this request.
         * @param content_length length of data that will be written to the server
         * @return true on success, otherwise false
         */
        virtual bool open(std::size_t content_length) = 0;

        /**
         * Completes the REST request and reads the status code/content length from the server. This must be called
         * after any POST payload and headers are written to the connection.
         * @return true on success, otherwise false
         */
        virtual bool send() = 0;

        /**
         * @return  -1 on error, otherwise number bytes read
         */
        virtual int read(char* buffer, int len) = 0;

        /**
         * @return  a negative status code on error, otherwise number bytes read
         */
        virtual int write(char const* buffer, int len) = 0;
    };
}

#endif //CHARGELAB_OPEN_FIRMWARE_REST_CONNECTION_INTERFACE_H
