set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

cmake_minimum_required(VERSION 3.7.2)
project("Diet Boom" VERSION 2.0.3 LANGUAGES C)

set(PACKAGE_STRING "${PROJECT_NAME} ${PROJECT_VERSION}")

if(MSVC)
    add_definitions("/D_CRT_SECURE_NO_WARNINGS" "/D_CRT_NONSTDC_NO_DEPRECATE")
endif()

find_package(SDL2)
find_package(SDL2_mixer)
find_package(SDL2_net)

configure_file(cmake/config.h.cin config.h)

add_subdirectory(Source)