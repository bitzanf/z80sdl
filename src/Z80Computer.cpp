//
// Created by Filip on 23.4.2023.
//

#include "Z80Computer.hpp"

Z80Computer::Z80Computer(const std::string& serialPort) :
        cpu(this),
        uart(serialPort),
        RAM(new uint8_t[0x6800]),
        ROM(new uint8_t[0x8000]),
        videoTextRAM(nullptr),
        videoAttrRAM(nullptr),
        videoRAMSize(0),
        videoRAMDirty(nullptr) {
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
    if (addr < 0x8000) return;  //we do not allow ROM writes

    auto self = (Z80Computer*)arg;
    uint8_t *ptr = self->addr2ptr(addr);
    if (ptr) {
        *ptr = val;
        if (self->videoRAMDirty != nullptr && addr >= 0x8000 && addr < 0x9800) *self->videoRAMDirty = true;
    }
}

uint8_t Z80Computer::cpu_in(void *arg, uint16_t port) {
    auto self = (Z80Computer*)arg;
    return 0;
}

void Z80Computer::cpu_out(void *arg, uint16_t port, uint8_t val) {
    auto self = (Z80Computer*)arg;
    switch (port) {
        case 0:
            self->videoAttrBankSwitch = val;
            break;
    }
}

uint8_t *Z80Computer::getRAM() {
    return RAM;
}

uint8_t *Z80Computer::getROM() {
    return ROM;
}

void Z80Computer::setVideoRAM(uint8_t *text, uint8_t *attr, uint16_t size) {
    if (size > 0x1800) throw std::runtime_error("Video RAM can't be larger than the allocated window (0x1800 bytes)!");

    videoTextRAM = text;
    videoAttrRAM = attr;
    videoRAMSize = size;
}

uint8_t *Z80Computer::addr2ptr(uint16_t addr) {
    if (addr < 0x8000) return &ROM[addr];
    if (addr < 0x8000 + videoRAMSize) return videoAttrBankSwitch ? &videoAttrRAM[addr - 0x8000] : &videoTextRAM[addr - 0x8000];
    if (addr >= 0x9800) return &RAM[addr - 0x9800];

    return nullptr;
}
