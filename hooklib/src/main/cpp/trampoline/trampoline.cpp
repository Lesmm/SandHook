//
// Created by SwiftGan on 2019/1/17.
//

#ifndef SANDLOCK_TRAMPOLINE_CPP
#define SANDLOCK_TRAMPOLINE_CPP

#include "../includes/trampoline.h"

namespace SandLock {

    class DirectJumpTrampoline : public Trampoline {
    public:

        DirectJumpTrampoline() : Trampoline::Trampoline() {}

        void setJumpTarget(Code target) {
            codeCopy(reinterpret_cast<Code>(&target), OFFSET_JUMP_ADDR_TARGET, BYTE_POINT);
        }

    protected:
        Size codeLength() override {
            return SIZE_DIRECT_JUMP_TRAMPOLINE;
        }

        Code templateCode() override {
            #if defined(__arm__)
            if (isThumbCode()) {
                return getThumbCodeAddress(reinterpret_cast<Code>(DIRECT_JUMP_TRAMPOLINE_T));
            } else {
                return reinterpret_cast<Code>(DIRECT_JUMP_TRAMPOLINE);
            }
            #else
            return reinterpret_cast<Code>(DIRECT_JUMP_TRAMPOLINE);
            #endif
        }
    };

    class ReplacementLockTrampoline : public Trampoline {
    public:

        void setLockMethod(Code hookMethod) {
            codeCopy(reinterpret_cast<Code>(&hookMethod), OFFSET_REPLACEMENT_ART_METHOD, BYTE_POINT);
            void* codeEntry = getEntryCodeAddr(hookMethod);
            codeCopy(reinterpret_cast<Code>(&codeEntry), OFFSET_REPLACEMENT_OFFSET_CODE_ENTRY, BYTE_POINT);
        }

    protected:
        Size codeLength() override {
            return SIZE_REPLACEMENT_LOCK_TRAMPOLINE;
        }

        Code templateCode() override {
            return reinterpret_cast<Code>(REPLACEMENT_LOCK_TRAMPOLINE);
        }
    };

    class InlineLockTrampoline : public Trampoline {
    public:

        void setOriginMethod(Code originMethod) {
            codeCopy(reinterpret_cast<Code>(&originMethod), OFFSET_INLINE_ORIGIN_ART_METHOD, BYTE_POINT);
            void* codeEntry = getEntryCodeAddr(originMethod);
            codeCopy(reinterpret_cast<Code>(&codeEntry), OFFSET_INLINE_ADDR_ORIGIN_CODE_ENTRY, BYTE_POINT);
        }

        void setLockMethod(Code hookMethod) {
            codeCopy(reinterpret_cast<Code>(&hookMethod), OFFSET_INLINE_LOCK_ART_METHOD, BYTE_POINT);
            void* codeEntry = getEntryCodeAddr(hookMethod);
            codeCopy(reinterpret_cast<Code>(&codeEntry), OFFSET_INLINE_ADDR_LOCK_CODE_ENTRY, BYTE_POINT);
        }

//        void setEntryCodeOffset(Size offSet) {
//            codeCopy(reinterpret_cast<Code>(&offSet), OFFSET_INLINE_OFFSET_ENTRY_CODE, BYTE_POINT);
//            #if defined(__arm__)
//            Code32Bit offset32;
//            offset32.code = offSet;
//            unsigned char offsetOP = isBigEnd() ? offset32.op.op2 : offset32.op.op1;
//            tweakOpImm(OFFSET_INLINE_OP_OFFSET_CODE, offsetOP);
//            #endif
//        }

        void setOriginCode(Code originCode) {
            codeCopy(originCode, OFFSET_INLINE_ORIGIN_CODE, SIZE_DIRECT_JUMP_TRAMPOLINE);
        }

        void setOriginCode(Code originCode, Size codeLen) {
            codeCopy(originCode, OFFSET_INLINE_ORIGIN_CODE, codeLen);
        }

        Code getCallOriginCode() {
            return reinterpret_cast<Code>((Size)getCode() + OFFSET_INLINE_ORIGIN_CODE);
        }

    protected:
        Size codeLength() override {
            return SIZE_INLINE_LOCK_TRAMPOLINE;
        }

        Code templateCode() override {
            #if defined(__arm__)
            if (isThumbCode()) {
                return getThumbCodeAddress(reinterpret_cast<Code>(INLINE_LOCK_TRAMPOLINE_T));
            } else {
                return reinterpret_cast<Code>(INLINE_LOCK_TRAMPOLINE);
            }
            #else
            return reinterpret_cast<Code>(INLINE_LOCK_TRAMPOLINE);
            #endif
        }
    };

    class CallOriginTrampoline : public Trampoline {
    public:

        void setOriginMethod(Code originMethod) {
            codeCopy(reinterpret_cast<Code>(&originMethod), OFFSET_CALL_ORIGIN_ART_METHOD, BYTE_POINT);
        }

        void setOriginCode(Code originCode) {
            codeCopy(reinterpret_cast<Code>(&originCode), OFFSET_CALL_ORIGIN_JUMP_ADDR, BYTE_POINT);
        }

    protected:
        Size codeLength() override {
            return SIZE_CALL_ORIGIN_TRAMPOLINE;
        }

        Code templateCode() override {
            #if defined(__arm__)
            if (isThumbCode()) {
                return getThumbCodeAddress(reinterpret_cast<Code>(CALL_ORIGIN_TRAMPOLINE_T));
            } else {
                return reinterpret_cast<Code>(CALL_ORIGIN_TRAMPOLINE);
            }
            #else
            return reinterpret_cast<Code>(CALL_ORIGIN_TRAMPOLINE);
            #endif
        }
    };

}

#endif //SANDLOCK_TRAMPOLINE_CPP
