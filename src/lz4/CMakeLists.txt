target_sources(Spectre
        PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/lz4.c"
        PUBLIC
            "${CMAKE_CURRENT_LIST_DIR}/lz4.h"
)