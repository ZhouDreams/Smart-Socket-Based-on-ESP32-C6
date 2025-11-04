idf_component_register(
    SRCS "http-server.c" "http-server.h"
    INCLUDE_DIRS "."
    REQUIRES app-wifi esp_wifi esp_event spiffs nvs_flash esp_http_server json
)