idf_component_register(
    SRCS "app-mqtt.c" "app-mqtt.h"
    INCLUDE_DIRS "."
    REQUIRES app-wifi lte4g bl0942 mqtt 
)