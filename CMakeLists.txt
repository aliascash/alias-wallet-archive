cmake_minimum_required(VERSION 3.4)

project(Spectrecoin
        VERSION 2.0.8
        LANGUAGES
        CXX
        )

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

# Include Boost as an imported target
find_package(Boost REQUIRED
        COMPONENTS
        chrono
        filesystem
        iostreams
        program-options
        system
        thread
        )
add_library(boost INTERFACE IMPORTED)
set_property(TARGET boost PROPERTY
        INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})

#target_link_libraries(example
#        Boost::chrono
#        Boost::filesystem
#        Boost::iostreams
#        Boost::program-options
#        Boost::system
#        Boost::thread
#        )

add_subdirectory(src)
