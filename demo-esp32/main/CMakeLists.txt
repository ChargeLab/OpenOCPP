# Custom function to gzip files
function(gzip_files)
    file(GLOB WEB_FILES "${CMAKE_CURRENT_SOURCE_DIR}/web/*.html" "${CMAKE_CURRENT_SOURCE_DIR}/web/*.ico")

    foreach(FILE ${WEB_FILES})
        get_filename_component(FILENAME ${FILE} NAME)
        set(GZIPPED_FILE "${CMAKE_CURRENT_SOURCE_DIR}/web/${FILENAME}.gz")

        add_custom_command(
                OUTPUT ${GZIPPED_FILE}
                COMMAND ${CMAKE_COMMAND} -E echo "Gzipping ${FILENAME}"
                COMMAND gzip -c ${FILE} > ${GZIPPED_FILE}
                DEPENDS ${FILE}
                VERBATIM
        )

        list(APPEND GZIPPED_FILES ${GZIPPED_FILE})
    endforeach()

    set(GZIPPED_FILES ${GZIPPED_FILES} PARENT_SCOPE)
endfunction()

# Call the function to generate gzipped files
gzip_files()

list(APPEND SOURCES "main.cc")
list(APPEND SOURCES "../../include/openocpp/implementation/logging_esp.cc")

idf_component_register(
        SRCS ${SOURCES}
        INCLUDE_DIRS "."
        REQUIRES app_update esp_websocket_client esp_wifi nvs_flash spiffs driver esp_eth esp_https_server esp_http_client
        EMBED_TXTFILES "certs/servercert.pem" "certs/prvtkey.pem" "certs/manufacturer.pem"
        EMBED_FILES
            "web/web_favicon.ico.gz"
            "web/web_index.html.gz"
            "web/web_setup.html.gz"
            "web/web_config.html.gz"
            "web/web_control.html.gz"
)
target_compile_options(${COMPONENT_LIB} PRIVATE -mtext-section-literals)

# add_compile_definitions(LOG_WITH_FILE_AND_LINE)
add_compile_definitions(JSON_NO_IO)

include_directories(../../include)
include_directories(../../rapidjson/include)
include_directories(${CMAKE_BINARY_DIR})
