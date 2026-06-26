cmake_minimum_required(VERSION 3.23)
project(OOP)

set(CMAKE_CXX_STANDARD 17)

set(SDL3_DIR "C:/Users/bilgi/CLionProjects/SDL3-3.4.10/x86_64-w64-mingw32")

include_directories(
        "${SDL3_DIR}/include"
        "${CMAKE_SOURCE_DIR}/include"
)

link_directories("${SDL3_DIR}/lib")

add_executable(OOP
        main.cpp
        src/StartMenu.cpp
        include/StartMenu.h
        include/AppState.h
        )


target_link_libraries(OOP SDL3)

add_custom_command(TARGET OOP POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_DIR}/bin/SDL3.dll"
        $<TARGET_FILE_DIR:OOP>
        )
