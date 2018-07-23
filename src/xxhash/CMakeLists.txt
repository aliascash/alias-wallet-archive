target_sources(Spectre
        PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/xxhash.c"
        PUBLIC
            "${CMAKE_CURRENT_LIST_DIR}/xxhash.h"
)