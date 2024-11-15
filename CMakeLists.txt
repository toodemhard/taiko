cmake_minimum_required (VERSION 3.11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project (taiko LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

option(TRACY_ENABLE OFF)

find_package(SDL3 REQUIRED)

if(SDL3_FOUND)
    message(STATUS "Found SDL3 at: ${SDL3_DIR}")
else()
    add_subdirectory(lib/sdl3 EXCLUDE_FROM_ALL)
endif()
#
# if (CMAKE_BUILD_TYPE STREQUAL "Release")
#     option(TRACY_ENABLE "" ON)
#     option(TRACY_ON_DEMAND "" ON)
# else()
# endif ()

add_subdirectory(lib/tracy EXCLUDE_FROM_ALL)
add_subdirectory(lib/EASTL EXCLUDE_FROM_ALL)

option(BUILD_SHARED_LIBS OFF)
add_subdirectory(lib/elzip)
add_subdirectory(lib/sdl-image EXCLUDE_FROM_ALL)

option(SDL3MIXER_OPUS "Enable Opus music" OFF)

add_subdirectory(lib/sdl-mixer EXCLUDE_FROM_ALL)
# add_subdirectory(lib/memory EXCLUDE_FROM_ALL)

set(SOURCE_DIRECTORY src)
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS ${SOURCE_DIRECTORY}/*.cpp)

# list(REMOVE_ITEM SOURCE_FILES 
#     "${CMAKE_CURRENT_SOURCE_DIR}/src/game.cpp"
#     "${CMAKE_CURRENT_SOURCE_DIR}/src/editor.cpp"
# )


add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if(MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:DEBUG>:/ZI>")
    target_link_options(${PROJECT_NAME} PUBLIC "/INCREMENTAL")
endif()

# if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
#     target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:DEBUG>:/ZI>")
#     target_link_options(${PROJECT_NAME} PUBLIC "/INCREMENTAL")

# endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
)

#target_compile_definitions(${PROJECT_NAME} PUBLIC
#    $<$<CONFIG:RELEASE>:TRACY_ENABLE>
#)
# target_include_directories(${PROJECT_NAME} PRIVATE src/)

target_include_directories(${PROJECT_NAME} PRIVATE
    lib/stb
    lib/cereal/include
)

target_link_libraries(${PROJECT_NAME} PUBLIC 
    Tracy::TracyClient
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    elzip
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_mixer::SDL3_mixer
    EASTL
)


add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}/data
)

add_dependencies(${PROJECT_NAME} copy_assets)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND_EXPAND_LISTS
)

option(COPY_TO_DISTRIBUTION "Copy application, libs, and data to clean distribution output dir" OFF)

if (COPY_TO_DISTRIBUTION)
    set(DISTRIBUTION_DIR ${CMAKE_SOURCE_DIR}/dist)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> ${DISTRIBUTION_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${DISTRIBUTION_DIR}
        COMMAND_EXPAND_LISTS
    )

    add_custom_target(copy_assets_to_dist
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${DISTRIBUTION_DIR}/data
    )

    add_dependencies(${PROJECT_NAME} copy_assets_to_dist)
endif()

# install(TARGETS ${PROJECT_NAME}
#     RUNTIME DESTINATION .
#     LIBRARY DESTINATION .
#     ARCHIVE DESTINATION .
# )
#
# install(DIRECTORY ${CMAKE_SOURCE_DIR}/data/ DESTINATION ${CMAKE_INSTALL_PREFIX}/data)
