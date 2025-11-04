idf_component_register(
    SRCS "button-and-relay.c" "button-and-relay.h"
    INCLUDE_DIRS "."
    REQUIRES driver app-mqtt
)