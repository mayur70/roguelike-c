cmake_minimum_required(VERSION 3.20)

project(roguelike LANGUAGES C)

add_subdirectory(vendor/SDL2 EXCLUDE_FROM_ALL)
add_subdirectory(vendor/SDL_image EXCLUDE_FROM_ALL)
add_subdirectory(vendor/SDL_ttf EXCLUDE_FROM_ALL)

add_executable(
    ${PROJECT_NAME}
    WIN32
    src/main.c
    src/panic.c
    src/tileset.c
    src/console.c
    src/app.c
    src/terminal.c
    src/events.c
    src/entity.c
    src/game_map.c
    src/fov.c
    src/astar.c 
    src/turn_log.c
    src/gameplay_state.c
    src/inventory.c
    src/items.c
 "src/savefile.h" "src/savefile.c")

target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)

# SDL2::SDL2main may or may not be available. It is e.g. required by Windows GUI applications
if(TARGET SDL2::SDL2main)
    # It has an implicit dependency on SDL2 functions, so it MUST be added before SDL2::SDL2 (or SDL2::SDL2-static)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 SDL2_image::SDL2_image SDL2_ttf::SDL2_ttf)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/res
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/res
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL2>
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL2_image>
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL2_ttf>
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror -Wpedantic)
endif()