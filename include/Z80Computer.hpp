//
// Created by Filip on 23.4.2023.
//

#ifndef Z80SDL_Z80COMPUTER_HPP
#define Z80SDL_Z80COMPUTER_HPP

//https://github.com/suzukiplan/z80
#include "ICs/z80.hpp"
#include "ICs/Intel8251.hpp"

/*
 * ---- MEMORY MAP ----
 * 0x0000:0x7FFF ROM
 * 0x8000:0x97FF VRAM
 * 0x9800:0xFFFF RAM
 * --------------------
 *
 * ------ IO MAP ------
 * 0 VRAM bank switch
 *   (0=text; 1=attr)
 * --------------------
 */

class Z80Computer {
public:
    explicit Z80Computer(const std::string& serialPort);
    ~Z80Computer();

    uint8_t *getRAM();
    uint8_t *getROM();
    void setVideoRAM(uint8_t *text, uint8_t *attr, uint16_t size);

    bool *videoRAMDirty;

    int runCycles(int cycles);

private:
    static uint8_t cpu_read(void *arg, uint16_t addr);
    static void cpu_write(void *arg, uint16_t addr, uint8_t val);
    static uint8_t cpu_in(void *arg, uint16_t port);
    static void cpu_out(void *arg, uint16_t port, uint8_t val);

    uint8_t *addr2ptr(uint16_t addr);

    Z80 cpu;
    Intel8251 uart;

    uint8_t *RAM, *ROM, *videoTextRAM, *videoAttrRAM;
    uint16_t videoRAMSize;
    bool videoAttrBankSwitch = false;
};


#endif //Z80SDL_Z80COMPUTER_HPP
