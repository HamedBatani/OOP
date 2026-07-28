cmake_minimum_required(VERSION 3.20)

project(OOP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(SDL3_DIR "" CACHE PATH "Directory containing SDL3Config.cmake")
if(NOT SDL3_DIR AND DEFINED ENV{SDL3_DIR})
    set(SDL3_DIR "$ENV{SDL3_DIR}" CACHE PATH "Directory containing SDL3Config.cmake" FORCE)
endif()
if(NOT SDL3_DIR AND DEFINED ENV{USERPROFILE})
    file(GLOB _sdl3_config_candidates
            "$ENV{USERPROFILE}/OneDrive/Desktop/SDL3*/x86_64-w64-mingw32/lib/cmake/SDL3"
            "$ENV{USERPROFILE}/Desktop/SDL3*/x86_64-w64-mingw32/lib/cmake/SDL3")
    if(_sdl3_config_candidates)
        list(GET _sdl3_config_candidates 0 _sdl3_discovered_dir)
        set(SDL3_DIR "${_sdl3_discovered_dir}" CACHE PATH "Directory containing SDL3Config.cmake" FORCE)
    endif()
endif()
find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3)

# معرفی فایل‌های سورس پروژه
# معرفی فایل‌های سورس پروژه (فایل‌های جدید بوم و رندرر اضافه شدند)
add_executable(OOP
        src/main.cpp
        src/Button.cpp
        src/StartMenu.cpp
        src/Canvas.cpp
        src/CanvasRenderer.cpp
        src/CircuitSimulator.cpp
        include/AppState.h
        include/PageSize.h
        include/Button.h
        include/StartMenu.h
        include/Canvas.h
        include/CanvasRenderer.h
        include/CircuitSimulator.h
        include/Grid.h
        include/Point.h
        src/Toolbar.cpp
        src/ComponentLibrary.cpp
        include/StatusBar.h
        src/StatusBar.cpp
        include/Toolbar.h
        include/ComponentLibrary.h
        include/PropertiesPanel.h
        include/Wire.h
        include/ProjectManager.h
        include/FirmwareLoader.h
        include/MicrocontrollerCore.h
        include/IOPort.h
        include/ExternalMemory.h
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

if(MSVC)
    target_compile_options(OOP PRIVATE /W4 /permissive-)
else()
    target_compile_options(OOP PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_custom_command(TARGET OOP POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:OOP>/data"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/data/DejaVuSans.ttf"
                "$<TARGET_FILE_DIR:OOP>/data/DejaVuSans.ttf")

include(CTest)
if(BUILD_TESTING)
    add_executable(OOP_core_tests
            tests/CoreTests.cpp
            src/CircuitSimulator.cpp
            src/ComponentLibrary.cpp)
    target_include_directories(OOP_core_tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3_ttf/include)
    target_link_directories(OOP_core_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL3_ttf/lib)
    target_link_libraries(OOP_core_tests PRIVATE SDL3::SDL3 SDL3_ttf)
    if(MSVC)
        target_compile_options(OOP_core_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OOP_core_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME core_simulation COMMAND OOP_core_tests)
endif()
