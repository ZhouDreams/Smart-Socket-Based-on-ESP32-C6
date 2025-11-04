idf_component_register(
    SRCS "app-wifi.c" "app-wifi.h"
    INCLUDE_DIRS "."
    REQUIRES app-mqtt driver esp_wifi esp_event nvs_flash lwip 
)