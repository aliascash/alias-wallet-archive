# *_lib are static, will not work in alpine since it does not use glibc, and
# hence:
#
#    async_posix.c:(.text+0x45): undefined reference to `getcontext'
#    /usr/lib/gcc/x86_64-alpine-linux-musl/8.2.0/../../../../x86_64-alpine-linux-musl/bin/ld: async_posix.c:(.text+0xb2): undefined reference to `makecontext'
#    collect2: error: ld returned 1 exit status

set(OPENSSL_LIBRARIES ssl_lib crypto_lib ${CMAKE_DL_LIBS})

mark_as_advanced(OPENSSL_INCLUDE_DIR OPENSSL_LIBRARIES)

# The following cannot be used due to messs in includes in libevent:
#
#    CMake Error in libevent/CMakeLists.txt:
#      Target "event_openssl_static" INTERFACE_INCLUDE_DIRECTORIES property
#      contains path:
#
#        "/bld/openssl-cmake/openssl-prefix/src/openssl/usr/local/include"
#
#      which is prefixed in the build directory.
#
## get_target_property(OPENSSL_INCLUDE_DIR ssl INTERFACE_INCLUDE_DIRECTORIES)

get_target_property(_openssl_include_dir ssl INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${_openssl_include_dir})
