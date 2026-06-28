# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/bilgi/CLionProjects/OOP/cmake-build-debug/_deps/sdl3-src"
  "C:/Users/bilgi/CLionProjects/OOP/cmake-build-debug/_deps/sdl3-build"
  "C:/Users/bilgi/CLionProjects/OOP/cmake-build-debug/_deps/sdl3-subbuild/sdl3-populate-prefix"
  "C:/Users/bilgi/CLionProjects/OOP/cmake-build-debug/_deps/sdl3-subbuild/sdl3-populate-prefix/tmp"
  "C:/Users/bilgi/CLionProjects/OOP/cmake-build-debug/_deps/sdl3-subbuild/sdl3-populate-prefix/src/sdl3-populate-stamp"
  "C:/Users/bilgi/CLionProjects/OOP/cmake-build-debug/_deps/sdl3-subbuild/sdl3-populate-prefix/src"
  "C:/Users/bilgi/CLionProjects/OOP/cmake-build-debug/_deps/sdl3-subbuild/sdl3-populate-prefix/src/sdl3-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/bilgi/CLionProjects/OOP/cmake-build-debug/_deps/sdl3-subbuild/sdl3-populate-prefix/src/sdl3-populate-stamp/${subDir}")
endforeach()
