cmake_minimum_required(VERSION 3.25)
project(z80sdl)

set(CMAKE_CXX_STANDARD 20)

add_executable(z80sdl src/main.cpp include/FTWrapper.hpp src/FTWrapper.cpp src/Z80Computer.cpp include/Z80Computer.hpp include/Z80Computer.hpp include/ICs/z80.hpp src/TextRenderer.cpp include/TextRenderer.hpp src/ICs/Intel8251.cpp include/ICs/Intel8251.hpp src/ICs/FPU.cpp include/ICs/FPU.hpp)

target_include_directories(z80sdl PRIVATE "include")

set(CMAKE_PREFIX_PATH extern)
FILE(GLOB ExternModules extern/*)
foreach(module ${ExternModules})
    add_subdirectory(${module})
endforeach()

add_definitions(-DSDL_MAIN_HANDLED)

find_package(Freetype REQUIRED)
find_package(ICU COMPONENTS data uc REQUIRED)

set(LIBRARIES
        SDL2pp::SDL2pp
        Freetype::Freetype
        ICU::uc
        ICU::data
        fmt::fmt-header-only
        fpm
        CLI11::CLI11
)

foreach (lib ${LIBRARIES})
    target_link_libraries(z80sdl PRIVATE ${lib})
endforeach()

add_subdirectory(z80)