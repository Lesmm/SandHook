//
// Created by swift on 2019/1/20.
//
#include "../includes/trampoline_manager.h"
#include "../includes/trampoline.h"
#include "../includes/inst.h"
#include "../includes/log.h"

extern int SDK_INT;
#define SWITCH_SETX0 false

namespace SandLock {


    uint32_t TrampolineManager::sizeOfEntryCode(mirror::ArtMethod *method) {
        Code codeEntry = getEntryCode(method);
        if (codeEntry == nullptr)
            return 0;
        #if defined(__arm__)
        if (isThumbCode(reinterpret_cast<Size>(codeEntry))) {
            codeEntry = getThumbCodeAddress(codeEntry);
        }
        #endif
        uint32_t size = *reinterpret_cast<uint32_t *>((Size)codeEntry - 4);
        return size;
    }

    class PCRelatedCheckVisitor : public InstVisitor {
    public:

        bool pcRelated = false;
        bool canSafeBackup = true;

        int instSize = 0;

        TrampolineManager* trampolineManager;

        PCRelatedCheckVisitor(TrampolineManager* t) {
            trampolineManager = t;
        }

        bool visit(Inst *inst, Size offset, Size length) override {

            instSize += inst->instLen();

            if (inst->pcRelated()) {
                LOGW("found pc related inst: %x !", inst->bin());
                if (trampolineManager->inlineSecurityCheck) {
                    pcRelated = true;
                    return false;
                }
            }

            if (instSize > SIZE_ORIGIN_PLACE_HOLDER) {
                canSafeBackup = false;
            }

            return true;
        }
    };

    class InstSizeNeedBackupVisitor : public InstVisitor {
    public:

        Size instSize = 0;

        bool visit(Inst *inst, Size offset, Size length) override {
            instSize += inst->instLen();
            return true;
        }
    };

    bool TrampolineManager::canSafeInline(mirror::ArtMethod *method) {

        if (skipAllCheck)
            return true;

        //check size
        if (method->isCompiled()) {
            uint32_t originCodeSize = sizeOfEntryCode(method);
            if (originCodeSize < SIZE_DIRECT_JUMP_TRAMPOLINE) {
                LOGW("can not inline due to origin code is too small(size is %d)", originCodeSize);
                return false;
            }
        }

        //check pc relate inst & backup inst len
        PCRelatedCheckVisitor visitor(this);

        InstDecode::decode(method->getQuickCodeEntry(), SIZE_DIRECT_JUMP_TRAMPOLINE, &visitor);

        return (!visitor.pcRelated) && visitor.canSafeBackup;
    }

    Code TrampolineManager::allocExecuteSpace(Size size) {
        if (size > EXE_BLOCK_SIZE)
            return 0;
        AutoLock autoLock(allocSpaceLock);
        void* mmapRes;
        Code exeSpace = 0;
        if (executeSpaceList.size() == 0) {
            goto label_alloc_new_space;
        } else if (executePageOffset + size > EXE_BLOCK_SIZE) {
            goto label_alloc_new_space;
        } else {
            exeSpace = executeSpaceList.back();
            Code retSpace = exeSpace + executePageOffset;
            executePageOffset += size;
            return retSpace;
        }
    label_alloc_new_space:
        mmapRes = mmap(NULL, EXE_BLOCK_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC,
                             MAP_ANON | MAP_PRIVATE, -1, 0);
        if (mmapRes == MAP_FAILED) {
            return 0;
        }
        memset(mmapRes, 0, EXE_BLOCK_SIZE);
        exeSpace = static_cast<Code>(mmapRes);
        executeSpaceList.push_back(exeSpace);
        executePageOffset = size;
        return exeSpace;
    }

    LockTrampoline* TrampolineManager::installReplacementTrampoline(mirror::ArtMethod *originMethod,
                                                                    mirror::ArtMethod *hookMethod,
                                                                    mirror::ArtMethod *backupMethod) {
        AutoLock autoLock(installLock);

        if (trampolines.count(originMethod) != 0)
            return getLockTrampoline(originMethod);
        LockTrampoline* hookTrampoline = new LockTrampoline();
        ReplacementLockTrampoline* replacementLockTrampoline = nullptr;
        CallOriginTrampoline* callOriginTrampoline = nullptr;
        Code replacementLockTrampolineSpace;
        Code callOriginTrampolineSpace;

        replacementLockTrampoline = new ReplacementLockTrampoline();
        replacementLockTrampoline->init();
        replacementLockTrampolineSpace = allocExecuteSpace(replacementLockTrampoline->getCodeLen());
        if (replacementLockTrampolineSpace == 0) {
            LOGE("hook error due to can not alloc execute space!");
            goto label_error;
        }
        replacementLockTrampoline->setExecuteSpace(replacementLockTrampolineSpace);
        replacementLockTrampoline->setEntryCodeOffset(quickCompileOffset);
        replacementLockTrampoline->setLockMethod(reinterpret_cast<Code>(hookMethod));
        hookTrampoline->replacement = replacementLockTrampoline;
        hookTrampoline->originCode = static_cast<Code>(originMethod->getQuickCodeEntry());

        if (SWITCH_SETX0 && SDK_INT >= ANDROID_N && backupMethod != nullptr) {
            callOriginTrampoline = new CallOriginTrampoline();
            checkThumbCode(callOriginTrampoline, getEntryCode(originMethod));
            callOriginTrampoline->init();
            callOriginTrampolineSpace = allocExecuteSpace(callOriginTrampoline->getCodeLen());
            if (callOriginTrampolineSpace == 0)
                goto label_error;
            callOriginTrampoline->setExecuteSpace(callOriginTrampolineSpace);
            callOriginTrampoline->setOriginMethod(reinterpret_cast<Code>(originMethod));
            Code originCode = getEntryCode(originMethod);
            if (callOriginTrampoline->isThumbCode()) {
                originCode = callOriginTrampoline->getThumbCodePcAddress(originCode);
            }
            callOriginTrampoline->setOriginCode(originCode);
            hookTrampoline->callOrigin = callOriginTrampoline;
        }

        trampolines[originMethod] = hookTrampoline;
        return hookTrampoline;

    label_error:
        delete hookTrampoline;
        delete replacementLockTrampoline;
        if (callOriginTrampoline != nullptr)
            delete callOriginTrampoline;
        return nullptr;
    }

    LockTrampoline* TrampolineManager::installInlineTrampoline(mirror::ArtMethod *originMethod,
                                                               mirror::ArtMethod *hookMethod,
                                                               mirror::ArtMethod *backupMethod) {

        AutoLock autoLock(installLock);

        if (trampolines.count(originMethod) != 0)
            return getLockTrampoline(originMethod);
        LockTrampoline* hookTrampoline = new LockTrampoline();
        InlineLockTrampoline* inlineLockTrampoline = nullptr;
        DirectJumpTrampoline* directJumpTrampoline = nullptr;
        CallOriginTrampoline* callOriginTrampoline = nullptr;
        Code inlineLockTrampolineSpace;
        Code callOriginTrampolineSpace;
        Code originEntry;
        Size sizeNeedBackup = SIZE_DIRECT_JUMP_TRAMPOLINE;
        InstSizeNeedBackupVisitor instVisitor;

        InstDecode::decode(originMethod->getQuickCodeEntry(), SIZE_DIRECT_JUMP_TRAMPOLINE, &instVisitor);
        sizeNeedBackup = instVisitor.instSize;

        //生成二段跳板
        inlineLockTrampoline = new InlineLockTrampoline();
        checkThumbCode(inlineLockTrampoline, getEntryCode(originMethod));
        inlineLockTrampoline->init();
        inlineLockTrampolineSpace = allocExecuteSpace(inlineLockTrampoline->getCodeLen());
        if (inlineLockTrampolineSpace == 0) {
            LOGE("hook error due to can not alloc execute space!");
            goto label_error;
        }
        inlineLockTrampoline->setExecuteSpace(inlineLockTrampolineSpace);
        inlineLockTrampoline->setEntryCodeOffset(quickCompileOffset);
        inlineLockTrampoline->setOriginMethod(reinterpret_cast<Code>(originMethod));
        inlineLockTrampoline->setLockMethod(reinterpret_cast<Code>(hookMethod));
        if (inlineLockTrampoline->isThumbCode()) {
            inlineLockTrampoline->setOriginCode(inlineLockTrampoline->getThumbCodeAddress(getEntryCode(originMethod)), sizeNeedBackup);
        } else {
            inlineLockTrampoline->setOriginCode(getEntryCode(originMethod), sizeNeedBackup);
        }
        hookTrampoline->inlineSecondory = inlineLockTrampoline;

        //注入 EntryCode
        directJumpTrampoline = new DirectJumpTrampoline();
        checkThumbCode(directJumpTrampoline, getEntryCode(originMethod));
        directJumpTrampoline->init();
        originEntry = getEntryCode(originMethod);
        if (!memUnprotect(reinterpret_cast<Size>(originEntry), directJumpTrampoline->getCodeLen())) {
            LOGE("hook error due to can not write origin code!");
            goto label_error;
        }

        if (directJumpTrampoline->isThumbCode()) {
            originEntry = directJumpTrampoline->getThumbCodeAddress(originEntry);
        }

        directJumpTrampoline->setExecuteSpace(originEntry);
        directJumpTrampoline->setJumpTarget(inlineLockTrampoline->getCode());
        hookTrampoline->inlineJump = directJumpTrampoline;

        //备份原始方法
        if (backupMethod != nullptr) {
            callOriginTrampoline = new CallOriginTrampoline();
            checkThumbCode(callOriginTrampoline, getEntryCode(originMethod));
            callOriginTrampoline->init();
            callOriginTrampolineSpace = allocExecuteSpace(callOriginTrampoline->getCodeLen());
            if (callOriginTrampolineSpace == 0) {

                goto label_error;
            }
            callOriginTrampoline->setExecuteSpace(callOriginTrampolineSpace);
            callOriginTrampoline->setOriginMethod(reinterpret_cast<Code>(originMethod));
            Code originCode = nullptr;
            if (callOriginTrampoline->isThumbCode()) {
                originCode = callOriginTrampoline->getThumbCodePcAddress(inlineLockTrampoline->getCallOriginCode());
                #if defined(__arm__)
                Code originRemCode = callOriginTrampoline->getThumbCodePcAddress(originEntry + sizeNeedBackup);
                Size offset = originRemCode - getEntryCode(originMethod);
                if (offset != directJumpTrampoline->getCodeLen()) {
                    Code32Bit offset32;
                    offset32.code = offset;
                    uint8_t offsetOP = callOriginTrampoline->isBigEnd() ? offset32.op.op4 : offset32.op.op1;
                    inlineLockTrampoline->tweakOpImm(OFFSET_INLINE_OP_ORIGIN_OFFSET_CODE, offsetOP);
                }
                #endif
            } else {
                originCode = inlineLockTrampoline->getCallOriginCode();
            }
            callOriginTrampoline->setOriginCode(originCode);
            hookTrampoline->callOrigin = callOriginTrampoline;
        }
        trampolines[originMethod] = hookTrampoline;
        return hookTrampoline;

    label_error:
        delete hookTrampoline;
        if (inlineLockTrampoline != nullptr) {
            delete inlineLockTrampoline;
        }
        if (directJumpTrampoline != nullptr) {
            delete directJumpTrampoline;
        }
        if (callOriginTrampoline != nullptr) {
            delete callOriginTrampoline;
        }
        return nullptr;
    }

    LockTrampoline* TrampolineManager::installNativeLockTrampolineNoBackup(void *origin,
                                                                           void *hook) { LockTrampoline* hookTrampoline = new LockTrampoline();
        DirectJumpTrampoline* directJumpTrampoline = new DirectJumpTrampoline();

        if (!memUnprotect(reinterpret_cast<Size>(origin), directJumpTrampoline->getCodeLen())) {
            LOGE("hook error due to can not write origin code!");
            goto label_error;
        }

        directJumpTrampoline->init();

        #if defined(__arm__)
        checkThumbCode(directJumpTrampoline, reinterpret_cast<Code>(origin));
        if (directJumpTrampoline->isThumbCode()) {
            origin = directJumpTrampoline->getThumbCodeAddress(reinterpret_cast<Code>(origin));
        }
        if (isThumbCode(reinterpret_cast<Size>(hook))) {
            hook = directJumpTrampoline->getThumbCodePcAddress(reinterpret_cast<Code>(hook));
        }
        #endif

        directJumpTrampoline->setExecuteSpace(reinterpret_cast<Code>(origin));
        directJumpTrampoline->setJumpTarget(reinterpret_cast<Code>(hook));
        hookTrampoline->inlineJump = directJumpTrampoline;
        directJumpTrampoline->flushCache(reinterpret_cast<Size>(origin), directJumpTrampoline->getCodeLen());
        hookTrampoline->hookNative = directJumpTrampoline;
        return hookTrampoline;

    label_error:
        delete hookTrampoline;
        delete directJumpTrampoline;
        return nullptr;
    }

    TrampolineManager &TrampolineManager::get() {
        static TrampolineManager trampolineManager;
        return trampolineManager;
    }

}
