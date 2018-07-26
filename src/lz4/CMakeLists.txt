target_sources(Spectre
        PUBLIC
            "${CMAKE_CURRENT_LIST_DIR}/lz4.h"
        PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/lz4.c"
)