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
#include "src/codegen/arm64/jit-code-signer-hybrid.h"
#include <sstream>

namespace v8 {
namespace internal {
JitCodeSignerHybrid::JitCodeSignerHybrid()
{
    Reset();
}

void JitCodeSignerHybrid::Reset()
{
    tmp_buffer_ = nullptr;
    ctx_.InitSalt();
    ctx_.Init(0);
    ctx_inited_ = true;
    skip_size_ = 0;
    offset_ = 0;
    sign_table_.clear();
    skipped_offset_.clear();
    while (!will_sign_.empty()) {
        will_sign_.pop();
    }
}

void JitCodeSignerHybrid::SignInstruction(Instr insn)
{
#ifdef JIT_CODE_SIGN_DEBUGGABLE
    if (static_cast<size_t>(GetIndexFromOffset(offset_)) != sign_table_.size()) {
        LOG_ERROR("Index = %d not equal signtable size = %zu.",
            GetIndexFromOffset(offset_), sign_table_.size());
    }
#endif
    if (skip_size_ > 0) {
        skipped_offset_.push_back(offset_);
        sign_table_.push_back(ctx_.SignSingle(insn, GetIndexFromOffset(offset_)));
        skip_size_ -= 1;
    } else {
        if (!ctx_inited_) {
            ctx_.Init(GetIndexFromOffset(offset_));
            ctx_inited_ = true;
        }
        uint32_t signature = ctx_.Update(insn);
        sign_table_.push_back(signature);
    }
    offset_ += INSTRUCTION_SIZE;
}

void JitCodeSignerHybrid::SkipNext(uint32_t n)
{
    skip_size_ = std::max(skip_size_, n);
    ctx_inited_ = false;
}

int32_t JitCodeSignerHybrid::PatchInstruction(int offset, Instr insn)
{
#ifdef JIT_CODE_SIGN_DEBUGGABLE
    if (std::find(skipped_offset_.begin(), skipped_offset_.end(), offset)
        == skipped_offset_.end()) {
        LOG_ERROR("Update no skipped instruction failed at offset" \
            "= %x", offset);
    }
#endif
    int cur_index = 0;
    if (!ConvertPatchOffsetToIndex(offset, cur_index)) {
        return CS_ERR_PATCH_INVALID;
    }
    int signature = ctx_.SignSingle(insn, cur_index);
    sign_table_[cur_index] = signature;
    return CS_SUCCESS;
}

int32_t JitCodeSignerHybrid::ValidateSubCode(Instr *jit_memory, PACSignCtx &verify_ctx,
    void *jit_buffer, int pos, int size)
{
    if (size == 0) {
        return CS_SUCCESS;
    }
#if defined(JIT_CODE_SIGN_DEBUGGABLE)
    LOG_INFO("Validate start = %p, offset = %x, size = %d",
        jit_buffer, pos, size);
#endif
    int32_t index = GetIndexFromOffset(pos);
    verify_ctx.Init(index);
    auto insn_ptr = reinterpret_cast<const Instr *>(
        reinterpret_cast<uintptr_t>(jit_buffer) + pos);
    while (size > 0) {
        uint32_t signature = verify_ctx.Update(*insn_ptr);
        if (signature != sign_table_[index]) {
#ifdef JIT_CODE_SIGN_DEBUGGABLE
            LOG_ERROR("Validate insn (%8x) failed at offset = %x, " \
                "signature(%x) != wanted(%{pucblic}x)",
                *(insn_ptr), index * INSTRUCTION_SIZE, signature, sign_table_[index]);
#endif
#ifndef JIT_CODE_SIGN_PERMISSIVE
            return CS_ERR_VALIDATE_CODE;
#else
            break;
#endif
        }
        *(jit_memory + index) = *insn_ptr;
        index++;
        insn_ptr++;
        size -= INSTRUCTION_SIZE;
    }
    return CS_SUCCESS;
}

int32_t JitCodeSignerHybrid::ValidateCodeCopy(Instr *jit_memory,
    void *tmp_buffer, int size)
{
    int32_t ret = CheckDataCopy(jit_memory, tmp_buffer, size);
    if (ret != CS_SUCCESS) {
        return ret;
    }

    PACSignCtx verify_ctx(CTXConfig::SIGN_NO_AUTH, ctx_.GetSalt());
    int offset = 0;
    for (uint32_t i = 0; i < skipped_offset_.size(); i++) {
        if (ValidateSubCode(jit_memory, verify_ctx, tmp_buffer_, offset,
            skipped_offset_[i] - offset) != CS_SUCCESS) {
            return CS_ERR_VALIDATE_CODE;
        }

        int32_t index = GetIndexFromOffset(skipped_offset_[i]);
        Instr insn = *reinterpret_cast<const Instr *>(reinterpret_cast<uintptr_t>(tmp_buffer_) \
            + skipped_offset_[i]);
        uint32_t signature = verify_ctx.SignSingle(insn, index);
        if (signature != sign_table_[index]) {
#ifdef JIT_CODE_SIGN_DEBUGGABLE
            LOG_ERROR("Validate insn (%x) without context failed at index = %x," \
                "signature(%x) != wanted(%{pucblic}x).",
                insn, index, signature, sign_table_[index]);
#endif
#ifndef JIT_CODE_SIGN_PERMISSIVE
            return CS_ERR_VALIDATE_CODE;
#endif
        }
        *(jit_memory + index) = insn;
        offset = skipped_offset_[i] + INSTRUCTION_SIZE;
    }

    if (ValidateSubCode(jit_memory, verify_ctx, tmp_buffer_,
        offset, size - offset) != CS_SUCCESS) {
        return CS_ERR_VALIDATE_CODE;
    }
    return CS_SUCCESS;
}
}
}