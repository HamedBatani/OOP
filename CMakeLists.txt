cmake_minimum_required(VERSION 3.20)

project(OOP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SDL3_DIR "C:/Users/bilgi/OneDrive/Desktop/SDL3-devel-3.4.10-mingw/SDL3-3.4.10/x86_64-w64-mingw32/lib/cmake/SDL3")
find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3)

# معرفی فایل‌های سورس پروژه
add_executable(OOP
        src/main.cpp
        src/Button.cpp
        src/StartMenu.cpp
        include/AppState.h
        include/PageSize.h
        include/Button.h
        include/StartMenu.h
        )

# تنظیم مسیر فایل‌های Header (هم برای خود پروژه و هم برای SDL3_ttf)
# کلمه libs به lib تغییر یافت
target_include_directories(OOP PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3_ttf/include
        )

# تنظیم مسیر فایل‌های Library برای SDL3_ttf
# کلمه libs به lib تغییر یافت
target_link_directories(OOP PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3_ttf/lib
        )

# لینک کردن کتابخانه‌ها به پروژه
target_link_libraries(OOP PRIVATE
        SDL3::SDL3
        SDL3_ttf
        )
