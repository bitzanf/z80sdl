//
// Created by filip on 18.7.23.
//

#ifndef Z80SDL_INTEL8251_HPP
#define Z80SDL_INTEL8251_HPP

#include <string>
#include <fstream>

//https://map.grauw.nl/resources/midi/intel_8251.pdf

class Intel8251 {
public:
    explicit Intel8251(const std::string& mountpoint);
    ~Intel8251();

    void transmit(uint8_t data);
    uint8_t receive();

    void writeInstruction(uint8_t instr);
    uint8_t getStatus();

private:
    void reset();

    union ModeInstruction {
        struct {
            uint8_t baudRate : 2;
            uint8_t charLength : 2;
            bool parityEnable : 1;
            bool parityEven_Odd : 1;
            uint8_t nStopBits : 2;
        } async;

        struct {
            uint8_t baudRate : 2;
            uint8_t charLength : 2;
            bool parityEnable : 1;
            bool parityEven_Odd : 1;
            bool extSyncDet : 1;
            bool singleCharSync : 1;
        } sync;

        uint8_t as_byte;
    };

    union CommandInstruction {
        struct {
            bool txEnable: 1;
            bool DTR: 1;
            bool rxEnable: 1;
            bool sendBreak: 1;
            bool errorReset: 1;
            bool RTS: 1;
            bool reset: 1;
            bool huntMode: 1;
        };

        uint8_t as_byte;
    };

    union DeviceStatus {
        struct {
            bool txReady : 1;
            bool rxReady : 1;
            bool txEmpty : 1;
            bool parityError : 1;
            bool overrunError : 1;
            bool framingError : 1;
            bool SYN_BRK_det : 1;
            bool DSR : 1;
        };

        uint8_t as_byte;
    };

    CommandInstruction commandRegister;
    ModeInstruction modeRegister;
    DeviceStatus status;
    bool modeSet;

    std::fstream mount;
};


#endif //Z80SDL_INTEL8251_HPP
