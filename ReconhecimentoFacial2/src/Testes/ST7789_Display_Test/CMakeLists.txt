cmake_minimum_required(VERSION 3.10)
project(st7789_display)
add_executable(st7789_display main.cpp ST7789.cpp)
target_link_libraries(st7789_display bcm2835)
