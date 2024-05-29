/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef V8_CODEGEN_ARM64_JIT_CODE_SIGNER_HYBRID_H
#define V8_CODEGEN_ARM64_JIT_CODE_SIGNER_HYBRID_H

#include <queue>
#include <vector>
#include "src/codegen/arm64/jit-code-signer-base.h"

namespace v8 {
namespace internal {

class JitCodeSignerHybrid : public JitCodeSignerBase {
public:
    JitCodeSignerHybrid();
    ~JitCodeSignerHybrid() {}
    void Reset();
    void SignInstruction(Instr insn);
    void SkipNext(uint32_t n);
    int32_t PatchInstruction(int offset, Instr insn);
    int32_t ValidateCodeCopy(Instr *jit_memory, void *jit_buffer, int size);
private:
    int32_t ValidateSubCode(Instr *jit_memory, PACSignCtx &verifyCtx,
        void *jit_buffer, int pos, int size);

    std::vector<int> skipped_offset_;
    uint32_t skip_size_;
    bool ctx_inited_;
};
}
}
#endif