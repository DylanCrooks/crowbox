# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.c)

# Vision link wire format, shared with the vision board's firmware (vision/).
list(APPEND app_sources ${CMAKE_SOURCE_DIR}/shared/vision_protocol.c)

idf_component_register(SRCS ${app_sources}
    INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/shared
    REQUIRES nvs_flash esp_system driver esp_driver_uart esp_timer)
