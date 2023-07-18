cmake_minimum_required(VERSION 3.25)
project(z80sdl)

set(CMAKE_CXX_STANDARD 20)

add_executable(z80sdl src/main.cpp include/FTWrapper.hpp src/FTWrapper.cpp src/Z80Computer.cpp include/Z80Computer.hpp include/Z80Computer.hpp include/ICs/z80.hpp src/TextRenderer.cpp include/TextRenderer.hpp src/ICs/Intel8251.cpp include/ICs/Intel8251.hpp src/ICs/FPU.cpp include/ICs/FPU.hpp)

if(WIN32)
    # -DCMAKE_TOOLCHAIN_FILE=g:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake
    target_include_directories(z80sdl PRIVATE "g:/dev/vcpkg/installed/x64-windows/include/")
endif()

target_include_directories(z80sdl PRIVATE "include")

find_package(SDL2 REQUIRED)
target_include_directories(z80sdl PRIVATE ${SDL2_INCLUDE_DIR})
target_link_libraries(z80sdl PRIVATE ${SDL2_LIBRARY})

find_package(SDL2PP REQUIRED)
target_include_directories(z80sdl PRIVATE ${SDL2PP_INCLUDE_DIRS})
target_link_libraries(z80sdl PRIVATE ${SDL2PP_LIBRARIES})

find_package(Freetype REQUIRED)
target_link_libraries(z80sdl PRIVATE Freetype::Freetype)

#find_package(ICU 72.1 COMPONENTS uc i18n REQUIRED)
find_package(ICU COMPONENTS data uc i18n REQUIRED)
target_link_libraries(z80sdl PRIVATE ICU::uc ICU::i18n ICU::data)

find_package(fmt REQUIRED)
target_link_libraries(z80sdl PRIVATE fmt::fmt-header-only)