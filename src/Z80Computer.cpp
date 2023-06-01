//
// Created by Filip on 23.4.2023.
//

#include "Z80Computer.hpp"

Z80Computer::Z80Computer() : cpu(&mmu) {
    cpu.setupCallbackFP(MMU::cpu_read, MMU::cpu_write, MMU::cpu_in, MMU::cpu_out);

}

uint8_t MMU::cpu_read(void *arg, uint16_t addr) {
    MMU *self = (MMU*)arg;
    return 0;
}

void MMU::cpu_write(void *arg, uint16_t addr, uint8_t val) {
    MMU *self = (MMU*)arg;
}

uint8_t MMU::cpu_in(void *arg, uint16_t port) {
    MMU *self = (MMU*)arg;
    return 0;
}

void MMU::cpu_out(void *arg, uint16_t port, uint8_t val) {
    MMU *self = (MMU*)arg;
}
