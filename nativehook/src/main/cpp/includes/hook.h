//
// Created by swift on 2019/5/14.
//

#pragma once

#include <mutex>
#include <atomic>

#include "code_buffer.h"
#include "decoder.h"
#include "assembler.h"
#include "code_relocate.h"

typedef Addr REG;

namespace SandLock {
    namespace Lock {

        struct LockInfo {
            bool is_break_point;
            void *user_data;
            void *origin;
            void *replace;
            void *backup;
        };

        using BreakCallback = bool (*)(sigcontext*, void*);

        class InlineLock {
        public:
            //return == backup method
            virtual void *Lock(void *origin, void *replace) = 0;
            virtual bool BreakPoint(void *point, void (*callback)(REG[])) {
                return false;
            };
            virtual bool SingleBreakPoint(void *point, BreakCallback callback, void *data = nullptr) {
                return false;
            };
            virtual void *SingleInstLock(void *origin, void *replace) {
                return nullptr;
            };
            virtual bool ExceptionHandler(int num, sigcontext *context) {
                return false;
            };
        protected:

            virtual bool InitForSingleInstLock();

            bool inited = false;
            static CodeBuffer* backup_buffer;
            std::mutex hook_lock;

        private:
            using SigAct = int (*)(int, struct sigaction *, struct sigaction *);
            SigAct sigaction_backup = nullptr;
        public:
            static InlineLock* instance;
            struct sigaction old_sig_act{};
        };

    }
}