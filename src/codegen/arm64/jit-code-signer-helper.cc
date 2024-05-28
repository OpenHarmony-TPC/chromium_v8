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
#include <cstdio>
#include <sys/auxv.h>
#include <asm/hwcap.h>
#include "src/codegen/arm64/jit-code-signer-helper.h"
#include "src/codegen/arm64/jit-code-signer-base.h"
#include "src/codegen/arm64/jit-code-signer-hybrid.h"
#include "v8config.h"
namespace v8 {
namespace internal {

enum class JitCodeSignerStatus {
    UNINITIALIZED,
    SUPPORT,
    UNSUPPORT
};

static JitCodeSignerStatus g_jitCodeSignerStatus = JitCodeSignerStatus::UNINITIALIZED;

bool IsSupportJitCodeSigner() {
    if (g_jitCodeSignerStatus == JitCodeSignerStatus::UNINITIALIZED) {
        unsigned long hwcaps = getauxval(AT_HWCAP);
        if ((hwcaps & HWCAP_PACA) && (hwcaps & HWCAP_PACG)) {
            g_jitCodeSignerStatus = JitCodeSignerStatus::SUPPORT;
        } else {
            g_jitCodeSignerStatus = JitCodeSignerStatus::UNSUPPORT;
        }
    }
    return g_jitCodeSignerStatus == JitCodeSignerStatus::SUPPORT;
}

void TryRegisterTmpBuffer(JitCodeSignerBase *jit_code_signer, void *tmp_buffer)
{
    if (jit_code_signer != nullptr) {
        jit_code_signer->RegisterTmpBuffer(tmp_buffer);
    }
}

void TrySetCompileMode(JitCodeSignerBase *jit_code_signer, int32_t mode)
{
    if (jit_code_signer != nullptr) {
        jit_code_signer->SetCompileMode(static_cast<CompileMode>(mode));
    }
}

void TryReset(JitCodeSignerBase *jit_code_signer)
{
    if (jit_code_signer != nullptr) {
        jit_code_signer->Reset();
    }
}

void TrySignInstruction(JitCodeSignerBase *jit_code_signer, void *target, uint32_t value)
{
    if (jit_code_signer != nullptr) {
        V8_LIKELY(jit_code_signer->SignInstruction(target, value));
    }
}

void TrySignData(JitCodeSignerBase *jit_code_signer, void *target, const void *data, uint32_t size)
{
    if (jit_code_signer != nullptr) {
        V8_LIKELY(jit_code_signer->SignData(target, data, size) == 0);
    }
}

void TryPatchInstruction(JitCodeSignerBase *jit_code_signer,
    void *target, uint32_t value)
{
    if (jit_code_signer != nullptr) {
        V8_LIKELY(jit_code_signer->PatchInstruction(target, value) == 0);
    }
}

void TryPatchInstruction(JitCodeSignerBase *jit_code_signer,
    int offset, uint32_t value)
{
    if (jit_code_signer != nullptr) {
        V8_LIKELY(jit_code_signer->PatchInstruction(offset, value) == 0);
    }
}

void TryPatchData(JitCodeSignerBase *jit_code_signer,
    int offset, void *data, uint32_t size)
{
    if (jit_code_signer != nullptr) {
        V8_LIKELY(jit_code_signer->PatchData(offset, data, size) == 0);
    }
}

void TryPatchData(JitCodeSignerBase *jit_code_signer,
    void *address, void *data, uint32_t size)
{
    if (jit_code_signer != nullptr) {
        V8_LIKELY(jit_code_signer->PatchData(address, data, size) == 0);
    }
}

void TrySkipNext(JitCodeSignerBase *jit_code_signer, uint32_t n)
{
    if (jit_code_signer != nullptr) {
        jit_code_signer->SkipNext(n);
    }
}

void TryValidateCodeCopy(JitCodeSignerBase *jit_code_signer, void *jit_memory,
    void *tmp_buffer, int size) {
    if (jit_code_signer != nullptr) {
        V8_LIKELY(jit_code_signer->ValidateCodeCopy(reinterpret_cast<Instr *>(jit_memory),
           tmp_buffer, size));
    }
}
}
}