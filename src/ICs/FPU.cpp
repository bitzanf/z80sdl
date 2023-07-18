//
// Created by filip on 18.7.23.
//

#include "ICs/FPU.hpp"
#include <stdexcept>

uint8_t FPU::operand::bytes::read(FPU::ByteSelect byte) {
    switch (byte) {
        case B0: return b0;
        case B1: return b1;
        case B2: return b2;
        case B3: return b3;
        default: return 0;
    }
}

void FPU::operand::bytes::write(FPU::ByteSelect byte, uint8_t data) {
    switch (byte) {
        case B0: b0 = data;
        case B1: b1 = data;
        case B2: b2 = data;
        case B3: b3 = data;
    }
}

void FPU::writeDataA(FPU::ByteSelect byte, uint8_t data) {
    A.bytes.write(byte, data);
}

void FPU::writeDataB(FPU::ByteSelect byte, uint8_t data) {
    B.bytes.write(byte, data);
}

uint8_t FPU::readResult(FPU::ByteSelect byte) {
    _resultReady = false;
    return result.bytes.read(byte);
}

bool FPU::resultReady() {
    return _resultReady;
}

void FPU::writeOpCode(uint8_t op) {
    if ((opcode = op) == 0) return;
    
    if (mode.AType != mode.resultType) {
        A = A.convert(static_cast<Types>(mode.AType), static_cast<Types>(mode.resultType));
    }
    if (mode.BType != mode.resultType) {
        B = B.convert(static_cast<Types>(mode.BType), static_cast<Types>(mode.resultType));
    }

    switch (mode.resultType) {
        case ot_I32:
            calcI32();
            break;
        case ot_F32:
            calcF32();
            break;
        case ot_Q8_8:
            calcQ8_8();
            break;
        default:
            _resultReady = false;
            return;
    }

    _resultReady = true;
}

void FPU::writeMode(uint8_t _mode) {
    mode.as_byte = _mode;
}

FPU::operand FPU::operand::convert(FPU::Types from, FPU::Types to) const {
    FPU::operand _result; // NOLINT(cppcoreguidelines-pro-type-member-init)

    if (from == ot_I32) {
        if (to == ot_I32) _result.i32 = i32;
        else if (to == ot_F32) _result.f32 = i32;
        else if (to == ot_Q8_8) _result.q8_8 = Q8_8{i32};
        else throw std::runtime_error{"Incorrect FPU conversion parameters"};
    } else if (from == ot_F32) {
        if (to == ot_I32) _result.i32 = f32;
        else if (to == ot_F32) _result.f32 = f32;
        else if (to == ot_Q8_8) _result.q8_8 = Q8_8 {f32};
        else throw std::runtime_error{"Incorrect FPU conversion parameters"};
    } else if (from == ot_Q8_8) {
        if (to == ot_I32) _result.i32 = static_cast<int>(q8_8);
        else if (to == ot_F32) _result.f32 = static_cast<float>(q8_8);
        else if (to == ot_Q8_8) _result.q8_8 = q8_8;
        else throw std::runtime_error{"Incorrect FPU conversion parameters"};
    } else throw std::runtime_error{"Incorrect FPU conversion parameters"};

    return _result;
}

void FPU::calcI32() {
    switch (opcode) {
        case op_ADD:
            result.i32 = A.i32 + B.i32;
            break;
        case op_SUB:
            result.i32 = A.i32 - B.i32;
            break;
        case op_MUL:
            result.i32 = A.i32 * B.i32;
            break;
        case op_DIV:
            result.i32 = A.i32 / B.i32;
            break;
        case op_CONVERT:
            result.i32 = A.i32;
            break;
    }
}

void FPU::calcF32() {
    switch (opcode) {
        case op_ADD:
            result.f32 = A.f32 + B.f32;
            break;
        case op_SUB:
            result.f32 = A.f32 - B.f32;
            break;
        case op_MUL:
            result.f32 = A.f32 * B.f32;
            break;
        case op_DIV:
            result.f32 = A.f32 / B.f32;
            break;
        case op_CONVERT:
            result.f32 = A.f32;
            break;
    }
}

void FPU::calcQ8_8() {
    switch (opcode) {
        case op_ADD:
            result.q8_8 = A.q8_8 + B.q8_8;
            break;
        case op_SUB:
            result.q8_8 = A.q8_8 - B.q8_8;
            break;
        case op_MUL:
            result.q8_8 = A.q8_8 * B.q8_8;
            break;
        case op_DIV:
            result.q8_8 = A.q8_8 / B.q8_8;
            break;
        case op_CONVERT:
            result.q8_8 = A.q8_8;
            break;
    }
}
