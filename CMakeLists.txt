cmake_minimum_required(VERSION 3.10)
project(Win98UI VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_definitions(UNICODE _UNICODE)

if(MSVC)
    add_compile_options(/FS)
endif()

# Core library
add_library(w98ui STATIC src/w98ui.cpp)
target_include_directories(w98ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(w98ui PUBLIC user32 gdi32 comctl32 gdiplus msimg32 uxtheme)

# Subdirectory for the output
add_subdirectory(example)