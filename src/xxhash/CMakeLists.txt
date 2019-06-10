target_sources(spectrecoin_lib
        PUBLIC
            "${CMAKE_CURRENT_LIST_DIR}/xxhash.h"
        PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/xxhash.c"
)