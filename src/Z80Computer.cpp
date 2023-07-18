//
// Created by Filip on 23.4.2023.
//

#include "Z80Computer.hpp"

Z80Computer::Z80Computer(const std::string& serialPort) :
        cpu(this),
        uart(serialPort),
        RAM(new uint8_t[0x6000]),
        ROM(new uint8_t[0x8000]),
        videoTextRAM(nullptr),
        videoAttrRAM(nullptr),
        videoRAMSize(0) {
    cpu.setupCallbackFP(cpu_read, cpu_write, cpu_in, cpu_out);
}

Z80Computer::~Z80Computer() {
    delete[] RAM;
    delete[] ROM;
};

uint8_t Z80Computer::cpu_read(void *arg, uint16_t addr) {
    auto self = (Z80Computer*)arg;
    uint8_t *ptr = self->addr2ptr(addr);
    if (ptr) return *ptr;
    else return 0;
}

void Z80Computer::cpu_write(void *arg, uint16_t addr, uint8_t val) {
    auto self = (Z80Computer*)arg;
    uint8_t *ptr = self->addr2ptr(addr);
    if (ptr) *ptr = val;
}

uint8_t Z80Computer::cpu_in(void *arg, uint16_t port) {
    auto self = (Z80Computer*)arg;
    return 0;
}

void Z80Computer::cpu_out(void *arg, uint16_t port, uint8_t val) {
    auto self = (Z80Computer*)arg;
}

uint8_t *Z80Computer::getRAM() {
    return RAM;
}

uint8_t *Z80Computer::getROM() {
    return ROM;
}

void Z80Computer::setVideoRAM(uint8_t *text, uint8_t *attr, uint16_t size) {
    videoTextRAM = text;
    videoAttrRAM = attr;
    videoRAMSize = size;
}

uint8_t *Z80Computer::addr2ptr(uint16_t addr) {
    if (addr >= 0x0000 && addr < 0x8000) return &ROM[addr];
    if (addr >= 0x8000 && addr < 0x8000 + videoRAMSize) return &videoTextRAM[addr - 0x8000];
    if (addr >= 0x9000 && addr < 0x9000 + videoRAMSize) return &videoAttrRAM[addr - 0x9000];
    if (addr >= 0xA000) return &RAM[addr - 0xA000];

    return nullptr;
}
