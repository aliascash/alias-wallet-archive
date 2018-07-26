target_sources(Spectre
        PUBLIC
            "${CMAKE_CURRENT_LIST_DIR}/xxhash.h"
        PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/xxhash.c"
)