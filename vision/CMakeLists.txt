# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# esp_wifi_remote and esp_hosted read ESP_IDF_VERSION ("5.5") in their Kconfig. Terminals set it
# differently (or not at all), which silently drops the Wi-Fi config, so pin it from the IDF in use.
set(ENV{ESP_IDF_VERSION} "${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}")
project(vision)
