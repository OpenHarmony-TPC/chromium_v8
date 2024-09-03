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
#include <asm/hwcap.h>
#include <sys/syscall.h>
#include "src/codegen/arm64/jit-code-signer-helper.h"
#include "src/codegen/arm64/jit-code-signer-base.h"
#include "src/codegen/arm64/jit-code-signer-hybrid.h"
#include "v8config.h"
namespace v8 {
namespace internal {

#define JITFORT_PRCTL_OPTION 0x6a6974
#define JITFORT_CPU_FEATURES 7

static inline long Syscall(
    unsigned long n, unsigned long a, unsigned long b,
    unsigned long c, unsigned long d, unsigned long e)
{
    register unsigned long x8 __asm__("x8") = n;
    register unsigned long x0 __asm__("x0") = a;
    register unsigned long x1 __asm__("x1") = b;
    register unsigned long x2 __asm__("x2") = c;
    register unsigned long x3 __asm__("x3") = d;
    register unsigned long x4 __asm__("x4") = e;
    asm volatile("svc 0" : "=r"(x0) : "r"(x8), "0"(x0), "r"(x1), \
        "r"(x2), "r"(x3), "r"(x4) : "memory", "cc");
    return x0;
}

static long inline PrctlWrapper(
    int op, unsigned long a, unsigned long b = 0)
{
    return Syscall(SYS_prctl, op, a, b, 0, 0);
}

bool IsSupportJitCodeSigner() {
    unsigned long hwcaps = static_cast<unsigned long>(PrctlWrapper(
        JITFORT_PRCTL_OPTION, JITFORT_CPU_FEATURES, 0));
    if ((hwcaps & HWCAP_PACA) && (hwcaps & HWCAP_PACG)) {
        return true;
    }
    return false;
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