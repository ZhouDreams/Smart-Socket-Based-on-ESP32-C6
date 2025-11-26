idf_component_register(
    SRCS "bl0942.c" "bl0942.h"
    INCLUDE_DIRS "."
    REQUIRES driver nvs_flash button-and-relay
)