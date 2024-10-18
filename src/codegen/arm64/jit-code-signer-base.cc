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
#include "src/codegen/arm64/jit-code-signer-base.h"

#include <sstream>
namespace v8 {
namespace internal {

constexpr int32_t BYTE_BIT_SIZE = 8;
constexpr uint32_t UNALIGNMENT_MASK = 0x3;

inline static Instr GetOneInstrForQueue(std::queue<uint8_t> &queue)
{
    Instr insn = 0;
    int i = 0;
    while ((i < INSTRUCTION_SIZE) && !queue.empty()) {
        insn |= (queue.front() << (BYTE_BIT_SIZE * i));
        queue.pop();
        i++;
    }
    return insn;
}

JitCodeSignerBase::JitCodeSignerBase():mode_(CompileMode::APPEND) {}

void JitCodeSignerBase::RegisterTmpBuffer(void *tmp_buffer)
{
    tmp_buffer_ = tmp_buffer;
}

void JitCodeSignerBase::SetCompileMode(CompileMode mode)
{
    mode_ = mode;
}

int JitCodeSignerBase::SignInstruction(void *buffer, Instr insn)
{
    if (mode_ == CompileMode::APPEND) {
        SignInstruction(insn);
    } else {
        return PatchInstruction(buffer, insn);
    }
    return CS_SUCCESS;
    
}

int JitCodeSignerBase::SignData(void *buffer, const void *const data, uint32_t size)
{
    if (mode_ == CompileMode::APPEND) {
        SignData(reinterpret_cast<const void *const>(data), size);
    } else {
        return PatchData(buffer, reinterpret_cast<const void *const>(data), size);
    }
    return CS_SUCCESS;
}

int32_t JitCodeSignerBase::SignData(const void *const data, uint32_t size)
{
    if (data == nullptr) {
        return CS_ERR_INVALID_DATA;
    }
    uint32_t cur = 0;
    size_t unsigned_size = will_sign_.size();
    if ((unsigned_size == 0) && (size >= INSTRUCTION_SIZE)) {
        auto insnPtr = reinterpret_cast<const Instr *const>(data);
        while (cur + INSTRUCTION_SIZE <= size) {
            SignInstruction(*insnPtr);
            insnPtr++;
            cur += INSTRUCTION_SIZE;
        }
    }

    if (cur == size) {
        return CS_SUCCESS;
    }
    unsigned_size += size - cur;
    const uint8_t *dataPtr = reinterpret_cast<const uint8_t*>(data);
    while (cur < size) {
        will_sign_.push(*(dataPtr + cur));
        cur++;
    }

    while (unsigned_size >= INSTRUCTION_SIZE) {
        Instr insn = GetOneInstrForQueue(will_sign_);
        SignInstruction(insn);
        unsigned_size -= INSTRUCTION_SIZE;
    }
    return CS_SUCCESS;
}

int32_t JitCodeSignerBase::PatchInstruction(void *buffer, Instr insn)
{
    if ((buffer == nullptr) || (tmp_buffer_ == nullptr)) {
        return CS_ERR_PATCH_INVALID;
    }
    return PatchInstruction(static_cast<int>(
        reinterpret_cast<uintptr_t>(buffer) - reinterpret_cast<uintptr_t>(tmp_buffer_)), insn);
}

int32_t JitCodeSignerBase::PatchData(int offset, const void *const data, uint32_t size)
{
    if (size & UNALIGNMENT_MASK) {
        return CS_ERR_JIT_SIGN_SIZE;
    }
    if (data == nullptr) {
        return CS_ERR_INVALID_DATA;
    }
    auto insnPtr = reinterpret_cast<const Instr *const>(data);
    int ret = 0;
    for (uint32_t i = 0; i < size; i += INSTRUCTION_SIZE) {
        ret = PatchInstruction(offset + i, *insnPtr);
        if (ret != CS_SUCCESS) {
            return ret;
        }
        insnPtr += 1;
    }
    return CS_SUCCESS;
}

int32_t JitCodeSignerBase::PatchData(void *buffer, const void *const data, uint32_t size)
{
    if ((buffer == nullptr) || (tmp_buffer_ == nullptr)) {
        return CS_ERR_PATCH_INVALID;
    }
    return PatchData(static_cast<int>(
        reinterpret_cast<uintptr_t>(buffer) - reinterpret_cast<uintptr_t>(tmp_buffer_)),
        data, size);
}

bool JitCodeSignerBase::ConvertPatchOffsetToIndex(const int offset, int &cur_index)
{
    if ((offset < 0) || ((offset & UNALIGNMENT_MASK) != 0)) {
        return false;
    }
    cur_index = GetIndexFromOffset(offset);
    if (static_cast<size_t>(cur_index) >= sign_table_.size()) {
#ifdef JIT_CODE_SIGN_DEBUGGABLE
        LOG_ERROR("Offset is out of range, index = %d, signTable size = %zu",
            cur_index, sign_table_.size());
#endif
        return false;
    }
    return true;
}

int32_t JitCodeSignerBase::CheckDataCopy(Instr *jit_memory, void *tmp_buffer, int size)
{
    if (jit_memory == nullptr) {
        return CS_ERR_JIT_MEMORY;
    }
    if (tmp_buffer == nullptr) {
        return CS_ERR_TMP_BUFFER;
    }

    // update tmp buffer
    tmp_buffer_ = tmp_buffer;

    if (((size & UNALIGNMENT_MASK) != 0) ||
        (static_cast<uint32_t>(size) > sign_table_.size() * INSTRUCTION_SIZE)) {
#ifdef JIT_CODE_SIGN_DEBUGGABLE
        LOG_ERROR("Range invalid, size = %d, table size = %zu",
            size, sign_table_.size());
#endif
        return CS_ERR_JIT_SIGN_SIZE;
    }
    return CS_SUCCESS;
}
}
}