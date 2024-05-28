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

#ifndef V8_CODEGEN_ARM64_JIT_CODE_SIGN_HELPER_H_
#define V8_CODEGEN_ARM64_JIT_CODE_SIGN_HELPER_H_

#include "src/base/memory.h"

namespace v8 {
namespace internal {

class JitCodeSignerBase;

bool IsSupportJitCodeSigner();

void TryRegisterTmpBuffer(JitCodeSignerBase *jit_code_signer, void *tmp_buffer);

void TrySetCompileMode(JitCodeSignerBase *jit_code_signer, int32_t mode);

void TryReset(JitCodeSignerBase *jit_code_signer);

void TrySignInstruction(JitCodeSignerBase *jit_code_signer, void *target, uint32_t insn);

void TrySignData(JitCodeSignerBase *jit_code_signer, void *target, const void *data, uint32_t size);

void TryPatchInstruction(JitCodeSignerBase *jit_code_signer,
    void *target, uint32_t value);

void TryPatchInstruction(JitCodeSignerBase *jit_code_signer,
    int offset, uint32_t value);

void TryPatchData(JitCodeSignerBase *jit_code_signer,
    int offset, void *data, uint32_t size);

void TryPatchData(JitCodeSignerBase *jit_code_signer,
    void *address, void *data, uint32_t size);

void TrySkipNext(JitCodeSignerBase *jit_code_signer, uint32_t n);

void TryValidateCodeCopy(JitCodeSignerBase *jit_code_signer, void *jit_memory,
    void *tmp_buffer, int size);
}
}
#endif