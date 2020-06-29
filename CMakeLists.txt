cmake_minimum_required(VERSION 2.8.3)
project(Flair_Flea3_calibration)

add_compile_options(-std=c++11)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


set(SPINNAKER_INCLUDE_DIR "/usr/include/spinnaker" CACHE PATH "Spinnaker Include Directory")
set(SPINNAKER_LIB_DIR "/usr/lib" CACHE PATH "Spinnaker Libs Directory")

find_package(Boost 1.45.0 COMPONENTS filesystem) 

include_directories( 
    SYSTEM ${SPINNAKER_INCLUDE_DIR}
)
link_directories( ${SPINNAKER_LIB_DIR} )
add_subdirectory(src)
