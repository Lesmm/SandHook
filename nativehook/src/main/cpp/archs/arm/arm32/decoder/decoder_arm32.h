//
// Created by swift on 2019/5/23.
//

#ifndef SANDLOCK_DECODER_A32_H
#define SANDLOCK_DECODER_A32_H

#include "decoder.h"

namespace SandLock {
    namespace Decoder {

        class Arm32Decoder : public InstDecoder {
        public:
            void Disassemble(void *codeStart, Addr codeLen, InstVisitor &visitor,
                             bool onlyPcRelInst) override;
        public:
            static Arm32Decoder* instant;
        };

    }
}

#endif //SANDLOCK_DECODER_A32_H
