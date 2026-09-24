# OpenOCPP/CMakeLists.txt
add_library(ocpp INTERFACE)

target_include_directories(ocpp INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include
)