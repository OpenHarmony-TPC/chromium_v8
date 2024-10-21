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
#ifndef V8_CODEGEN_ARM64_JIT_CODE_SIGNER_BASE_H
#define V8_CODEGEN_ARM64_JIT_CODE_SIGNER_BASE_H

#include <queue>
#include <vector>
#include "src/codegen/arm64/pac-sign-ctx.h"
#include "src/codegen/arm64/instructions-arm64.h"

namespace v8 {
namespace internal {

constexpr int32_t INSTRUCTION_SIZE = 4;
constexpr int32_t LOG_2_INSTRUCTION_SIZE = 2;

static inline int GetIndexFromOffset(int offset)
{
    return offset >> LOG_2_INSTRUCTION_SIZE;
}

enum JitCodeSignErrCode {
    CS_SUCCESS = 0,
    CS_ERR_NO_SIGNER = -0x700,
    CS_ERR_PATCH_INVALID = -0x701,
    CS_ERR_JIT_SIGN_SIZE = -0x702,
    CS_ERR_TMP_BUFFER = -0x703,
    CS_ERR_VALIDATE_CODE = -0x704,
    CS_ERR_JITFORT_IN = -0x705,
    CS_ERR_JITFORT_OUT = -0x706,
    CS_ERR_SIGN_OFFSET = -0x707,
    CS_ERR_INVALID_DATA = -0x708,
    CS_ERR_JIT_MEMORY = -0x709,
};

enum class CompileMode : int32_t {
    APPEND,
    PATCH
};

class JitCodeSignerBase {
public:
    JitCodeSignerBase();
    virtual ~JitCodeSignerBase() {}
    virtual void Reset() = 0;
    virtual void SignInstruction(Instr insn) = 0;
    virtual void SkipNext(uint32_t n) = 0;
    virtual int32_t PatchInstruction(int offset, Instr insn) = 0;
    virtual int32_t ValidateCodeCopy(Instr *jit_memory, void *jit_buffer, int size) = 0;

    void SetCompileMode(CompileMode mode);
    int32_t SignInstruction(void *buffer, Instr insn);
    int32_t SignData(void *buffer, const void *const data, uint32_t size);
    void RegisterTmpBuffer(void *tmp_buffer);
    int32_t SignData(const void *data, uint32_t size);
    int32_t PatchInstruction(void *jit_buffer, Instr insn);
    int32_t PatchData(int offset, const void *const data, uint32_t size);
    int32_t PatchData(void *buffer, const void *const data, uint32_t size);

protected:
    bool ConvertPatchOffsetToIndex(const int offset, int &cur_index);
    int32_t CheckDataCopy(Instr *jit_memory, void *jit_buffer, int size);

protected:
    void *tmp_buffer_;
    int offset_;
    std::queue<uint8_t> will_sign_;
    std::vector<uint32_t> sign_table_;
    PACSignCtx ctx_;
    CompileMode mode_;
};
}
}
#endif