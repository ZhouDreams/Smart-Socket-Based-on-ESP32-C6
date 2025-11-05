idf_component_register(
    SRCS "lte4g.c" "lte4g.h"
    INCLUDE_DIRS "."
    REQUIRES driver button-and-relay app-mqtt
)