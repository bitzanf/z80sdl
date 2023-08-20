//
// Created by filip on 18.7.23.
//

#include "ICs/Intel8251.hpp"
#include <cstring>

// WRITE 3 '0x00' VALUES AS INSTRUCTION AFTER INITIAL POWER-UP
// - datasheet page 12

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-pro-type-member-init"
Intel8251::Intel8251(const std::string &mountpoint) {
    mount.open(mountpoint);
    if (mount.fail()) throw std::runtime_error(std::string("Intel8251 error: ") + strerror(errno));
    reset();
    status.DSR = true;
}
#pragma clang diagnostic pop

void Intel8251::reset() {
    commandRegister.as_byte = 0;
    modeRegister.as_byte = 0;
    status.as_byte = 0;
    modeSet = false;
}

void Intel8251::writeInstruction(uint8_t instr) {
    if (modeSet) {
        commandRegister.as_byte = instr;

        if (!commandRegister.txEnable) status.txEmpty = true;
        else status.txEmpty = false;

        if (commandRegister.reset) reset();
    } else {
        modeRegister.as_byte = instr;
        modeSet = true;
    }
}

uint8_t Intel8251::getStatus() {
    status.rxReady = !mount.eof();
    return status.as_byte;
}

void Intel8251::transmit(uint8_t data) {
    if (commandRegister.txEnable) {
        status.txReady = false;
        mount << (char)data;
        status.txReady = true;
    }
}

uint8_t Intel8251::receive() {
    char c = 0;
    if (commandRegister.rxEnable) mount >> c;
    return c;
}

Intel8251::~Intel8251() = default;

