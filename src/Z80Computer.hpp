//
// Created by Filip on 23.4.2023.
//

#ifndef Z80SDL_Z80COMPUTER_HPP
#define Z80SDL_Z80COMPUTER_HPP

//https://github.com/suzukiplan/z80
#include "z80.hpp"

class MMU {
public:
    static uint8_t cpu_read(void *arg, uint16_t addr);
    static void cpu_write(void *arg, uint16_t addr, uint8_t val);
    static uint8_t cpu_in(void *arg, uint16_t port);
    static void cpu_out(void *arg, uint16_t port, uint8_t val);
};

class Z80Computer {
public:
    Z80Computer();
private:
    Z80 cpu;
    MMU mmu;
};


#endif //Z80SDL_Z80COMPUTER_HPP
