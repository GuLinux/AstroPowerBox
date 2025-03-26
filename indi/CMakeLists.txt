# define the project name
cmake_minimum_required(VERSION 3.20)
project(indi-astropowerbox C CXX)


include(GNUInstallDirs)

# add our cmake_modules folder
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")

# find our required packages
find_package(INDI REQUIRED)

# set our include directories to look for header files
include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})

# include(CMakeCommon)

# tell cmake to build our executable
add_executable(
    indi_astropowerbox
    indi_astropowerbox.cpp
)

# and link it to these libraries
target_link_libraries(
    indi_astropowerbox
    ${INDI_LIBRARIES}
)

# tell cmake where to install our executable
install(TARGETS indi_astropowerbox RUNTIME DESTINATION bin)


