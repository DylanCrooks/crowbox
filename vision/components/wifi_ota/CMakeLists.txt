# The AP password lives in a gitignored header; fail early with instructions if it's missing.
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/wifi_ota_secrets.h")
    message(FATAL_ERROR "components/wifi_ota/wifi_ota_secrets.h is missing: copy "
                        "wifi_ota_secrets.example.h to it and set a password.")
endif()

idf_component_register(SRCS "wifi_ota.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES app_update esp_app_format esp_event esp_http_server
                                     esp_netif esp_timer esp_wifi nvs_flash)
