cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project (taiko LANGUAGES CXX)
enable_language(ASM_NASM ASM)

# # Dependencies
# set(RAYLIB_VERSION 5.0)
# find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
# if (NOT raylib_FOUND) # If there's none, fetch and build raylib
#   include(FetchContent)
#   FetchContent_Declare(
#     raylib
#     DOWNLOAD_EXTRACT_TIMESTAMP OFF
#     URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
#   )
#   FetchContent_GetProperties(raylib)
#   if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
#     set(FETCHCONTENT_QUIET NO)
#     FetchContent_Populate(raylib)
#     set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
#     add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
#   endif()
# endif()
set(DIST_DIR ${CMAKE_SOURCE_DIR}/dist)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${DIST_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DIST_DIR})

option(SDL3IMAGE_AVIF "Support loading AVIF images" OFF)
option(SDL3IMAGE_BMP "Support loading BMP images" OFF)
option(SDL3IMAGE_GIF "Support loading GIF images" OFF)
option(SDL3IMAGE_JPG "Support loading JPEG images" ON)
option(SDL3IMAGE_JXL "Support loading JXL images" OFF)
option(SDL3IMAGE_LBM "Support loading LBM images" OFF)
option(SDL3IMAGE_PCX "Support loading PCX images" OFF)
option(SDL3IMAGE_PNG "Support loading PNG images" ON)
option(SDL3IMAGE_PNM "Support loading PNM images" OFF)
option(SDL3IMAGE_QOI "Support loading QOI images" OFF)
option(SDL3IMAGE_SVG "Support loading SVG images" OFF)
option(SDL3IMAGE_TGA "Support loading TGA images" OFF)
option(SDL3IMAGE_TIF "Support loading TIFF images" OFF)
option(SDL3IMAGE_WEBP "Support loading WEBP images" OFF)
option(SDL3IMAGE_XCF "Support loading XCF images" OFF)
option(SDL3IMAGE_XPM "Support loading XPM images" OFF)
option(SDL3IMAGE_XV "Support loading XV images" OFF)

add_subdirectory(lib/sdl3)
add_subdirectory(lib/sdl-image)
add_subdirectory(lib/sdl-mixer)
add_subdirectory(lib/sdl-ttf)
#set(CMAKE_BUILD_TYPE Debug)
#add_compile_options(-Wall) #-Wextra -Wpedantic)

set(SOURCE_DIRECTORY src)
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS ${SOURCE_DIRECTORY}/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE ${SOURCE_DIRECTORY})
target_include_directories(${PROJECT_NAME} PRIVATE lib/minimp3)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PUBLIC "/ZI")
  target_link_options(${PROJECT_NAME} PUBLIC "/INCREMENTAL")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer SDL3_ttf::SDL3_ttf)
