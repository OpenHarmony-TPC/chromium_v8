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
#include "src/codegen/arm64/pac-sign-ctx.h"

#include <memory>
#include "src/base/utils/random-number-generator.h"

namespace v8 {
namespace internal {

constexpr uint64_t SIGN_WITH_CONTEXT_PREFIX = 0x2LL << 60;
constexpr uint64_t SIGN_WITHOUT_CONTEXT_PREFIX = 0x3LL << 60;
constexpr uint64_t AUTH_CONTEXT_PREFIX = 0x1LL << 60;
constexpr uint32_t HIGH_BITS_RIGHT_SHIFT = 32;

static inline uint64_t PACDB(uint64_t value, uint64_t modifier)
{
#ifdef ARCH_PAC_SUPPORT
    asm volatile("pacdb %0, %1" : "+r"(value) : "r"(modifier) :);
#endif
    return value;
}

static inline uint64_t AUTDB(uint64_t value, uint64_t modifier)
{
#ifdef ARCH_PAC_SUPPORT
    asm volatile("autdb %0, %1" : "+r"(value) : "r"(modifier) :);
#endif
    return value;
}

static inline uint32_t PACGA(uint64_t value, uint64_t modifier)
{
#ifdef ARCH_PAC_SUPPORT
    uint64_t ret = 0;
    asm volatile("pacga %0, %1, %2" : "=r"(ret) : "r"(value), "r"(modifier) :);
#else
    uint64_t ret = value;
#endif
    return static_cast<uint32_t>(ret >> HIGH_BITS_RIGHT_SHIFT);
}

PACSignCtx::PACSignCtx(CTXConfig config, uint32_t salt)
    : context_(0), salt_(salt), index_(0), config_(config) {}

PACSignCtx::~PACSignCtx() {}

void PACSignCtx::InitSalt()
{
    static base::RandomNumberGenerator random_number_generator;
    salt_ = static_cast<uint32_t>(random_number_generator.NextInt());
}

void PACSignCtx::Init(int index)
{
    index_ = index;
    SetContext(GetSalt());
}

uint64_t PACSignCtx::PaddingContext(ContextType type, int index)
{
    uint32_t context;
    uint64_t prefix;
    switch (type) {
        case SIGN_WITH_CONTEXT:
            context = static_cast<uint32_t>(context_);
            prefix = SIGN_WITH_CONTEXT_PREFIX;
            index = index_;
            break;
        case SIGN_WITHOUT_CONTEXT:
            context = GetSalt();
            prefix = SIGN_WITHOUT_CONTEXT_PREFIX;
            break;
        case AUTH_CONTEXT:
            context = GetSalt();
            index = index_;
            prefix = AUTH_CONTEXT_PREFIX;
            break;
        default:
            UNREACHABLE();
    }
#if defined(JIT_CODE_SIGN_DEBUGGABLE)
    LOG_INFO("Padding prefix = %lx, index = %x, context = %x",
        prefix, index, context);
#endif
    uint64_t ret = prefix | ((static_cast<uint64_t>(index) & 0xfffffff) << 32) | context;
    return ret;
}

void PACSignCtx::SetContext(uint32_t context)
{
    if (config_ == CTXConfig::SIGN_NO_AUTH) {
        context_ = context;
        return;
    }
    uint64_t padding_context = PaddingContext(AUTH_CONTEXT);
    context_ = PACDB(context, padding_context);
}

uint64_t PACSignCtx::GetRealContext()
{
    uint64_t padding_context = PaddingContext(AUTH_CONTEXT);
    return AUTDB(context_, padding_context);
}

uint32_t PACSignCtx::SignWithContext(uint32_t value)
{
    uint64_t padding_context = PaddingContext(SIGN_WITH_CONTEXT);
    return PACGA(value, padding_context);
}

uint32_t PACSignCtx::Update(uint32_t value)
{
#if defined(JIT_CODE_SIGN_DEBUGGABLE)
    LOG_INFO("Before update context = %lx", context_);
#endif
    if (config_ == CTXConfig::SIGN_AND_AUTH) {
        context_ = GetRealContext();
    }
    index_ += 1;
    uint32_t signature = SignWithContext(value);
    SetContext(signature);
#if defined(JIT_CODE_SIGN_DEBUGGABLE)
    LOG_INFO("After update context = %lx, signature = %x",
        context_, signature);
#endif
    return signature;
}

void PACSignCtx::Finalize()
{
    if (config_ == CTXConfig::SIGN_AND_AUTH) {
        (void) GetRealContext();
    }
}

uint32_t PACSignCtx::SignSingle(uint32_t value, uint32_t index)
{
    uint64_t padding_context = PaddingContext(SIGN_WITHOUT_CONTEXT, index);
    uint32_t signature = PACGA(value, padding_context);
#if defined(JIT_CODE_SIGN_DEBUGGABLE)
    LOG_INFO("Get signature = %x", signature);
#endif
    return signature;
}

uint32_t PACSignCtx::GetSalt()
{
    return salt_;
}
}
}