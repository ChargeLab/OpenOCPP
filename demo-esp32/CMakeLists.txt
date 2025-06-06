cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 17)

if (ESP_PLATFORM)
    add_compile_definitions(JSON_NO_IO)

    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    project(TestFirmware LANGUAGES C CXX)

    include_directories($ENV{IDF_PATH}/components/freertos/include/esp_additions/freertos/)
endif()
