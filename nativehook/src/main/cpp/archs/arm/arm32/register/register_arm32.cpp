//
// Created by swift on 2019/5/12.
//

#include "register_arm32.h"
#include "register_list_arm32.h"

using namespace SandLock::RegistersA32;

SandLock::Asm::RegisterA32::RegisterA32(U8 code) : Register(code) {
}

SandLock::Asm::RegisterA32::RegisterA32() {

}

U8 SandLock::Asm::RegisterA32::Wide() {
    return 4;
}

RegisterA32* RegisterA32::registers[ARM32_REGISTER_COUNT] = {
#define DEFINE_REGISTERS_X(N) \
        &R##N,
        ARM32_REGISTER_CODE_LIST(DEFINE_REGISTERS_X)
#undef DEFINE_REGISTERS_X
};