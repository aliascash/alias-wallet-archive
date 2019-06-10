target_sources(spectrecoin_lib
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/lz4.h

        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/lz4.c
        )
