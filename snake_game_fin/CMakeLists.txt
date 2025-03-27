cmake_minimum_required(VERSION 3.27)
project(snake_game_fin)

set(CMAKE_CXX_STANDARD 17)

link_libraries(libeasyx.a libgdi32.a libole32.a)

add_executable(snake_game_fin main.cpp)

target_link_libraries(snake_game_fin PRIVATE Winmm)