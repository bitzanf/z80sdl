//
// Created by filip on 18.7.23.
//

#ifndef Z80SDL_FPU_HPP
#define Z80SDL_FPU_HPP

#include <cstdint>
#include "fpm/fixed.hpp"    //https://github.com/MikeLankamp/fpm

/*
 * ---- OPCODES ----
 * 0 NOP
 * 1 ADD
 * 2 SUB
 * 3 MUL
 * 4 DIV
 * 5 CONVERT
 * -----------------
 *
 * Mode: [resultType | AType | BType | (ignored)] byte lsb->msb
 */

class FPU {
public:
    enum ByteSelect {
        B0, B1, B2, B3
    };

    FPU() = default;
    ~FPU() = default;

    void writeDataA(ByteSelect byte, uint8_t data);
    void writeDataB(ByteSelect byte, uint8_t data);
    void writeOpCode(uint8_t op);
    void writeMode(uint8_t _mode);

    uint8_t readResult(ByteSelect byte);
    bool resultReady();

private:
    using Q8_8 = fpm::fixed<int16_t, int32_t, 8>;

    enum Types {
        ot_I32 = 1, ot_F32 = 2, ot_Q8_8 = 3
    };

    union operand {
        float f32;
        int i32;
        Q8_8 q8_8;

        struct bytes {
            uint8_t b0, b1, b2, b3;

            void write(ByteSelect byte, uint8_t data);
            uint8_t read(ByteSelect byte);
        } bytes;

        [[nodiscard]] operand convert(Types from, Types to) const;
    };

    union Mode {
        struct {
            uint8_t resultType : 2;
            uint8_t AType : 2;
            uint8_t BType : 2;
        };

        uint8_t as_byte;
    };

    enum opcodes {
        op_NOP,
        op_ADD,
        op_SUB,
        op_MUL,
        op_DIV,
        op_CONVERT
    };

    void calcI32();
    void calcF32();
    void calcQ8_8();

    operand A, B, result;
    bool _resultReady;
    uint8_t opcode;
    Mode mode;
};

#endif //Z80SDL_FPU_HPP
