# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/Asus/CLionProjects/OOP/cmake-build-debug/_deps/sdl3_ttf-src"
  "C:/Users/Asus/CLionProjects/OOP/cmake-build-debug/_deps/sdl3_ttf-build"
  "C:/Users/Asus/CLionProjects/OOP/cmake-build-debug/_deps/sdl3_ttf-subbuild/sdl3_ttf-populate-prefix"
  "C:/Users/Asus/CLionProjects/OOP/cmake-build-debug/_deps/sdl3_ttf-subbuild/sdl3_ttf-populate-prefix/tmp"
  "C:/Users/Asus/CLionProjects/OOP/cmake-build-debug/_deps/sdl3_ttf-subbuild/sdl3_ttf-populate-prefix/src/sdl3_ttf-populate-stamp"
  "C:/Users/Asus/CLionProjects/OOP/cmake-build-debug/_deps/sdl3_ttf-subbuild/sdl3_ttf-populate-prefix/src"
  "C:/Users/Asus/CLionProjects/OOP/cmake-build-debug/_deps/sdl3_ttf-subbuild/sdl3_ttf-populate-prefix/src/sdl3_ttf-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/Asus/CLionProjects/OOP/cmake-build-debug/_deps/sdl3_ttf-subbuild/sdl3_ttf-populate-prefix/src/sdl3_ttf-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/Asus/CLionProjects/OOP/cmake-build-debug/_deps/sdl3_ttf-subbuild/sdl3_ttf-populate-prefix/src/sdl3_ttf-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
