cmake_minimum_required(VERSION 3.20)

project(OOP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SDL3_DIR "C:/Users/Asus/OneDrive/Desktop/SDL3-3.4.10/x86_64-w64-mingw32/lib/cmake/SDL3")
find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3)

# معرفی فایل‌های سورس پروژه (فایل‌های جدید بوم و رندرر اضافه شدند)
add_executable(OOP
        src/main.cpp
        src/Button.cpp
        src/StartMenu.cpp
        src/Canvas.cpp
        src/CanvasRenderer.cpp
        include/AppState.h
        include/PageSize.h
        include/Button.h
        include/StartMenu.h
        include/Canvas.h
        include/CanvasRenderer.h
        include/Grid.h
        include/Point.h
        include/GridLines.h
)

# تنظیم مسیر فایل‌های Header (هم برای خود پروژه و هم برای SDL3_ttf)
target_include_directories(OOP PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3_ttf/include
)

target_link_directories(OOP PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3_ttf/lib
)

target_link_libraries(OOP PRIVATE
        SDL3::SDL3
        SDL3_ttf
)