// Copyright 2022 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/wasm/assembler-buffer-cache.h"

#include <algorithm>

#include "src/codegen/assembler.h"
#include "src/wasm/wasm-engine.h"

#ifdef V8_ENABLE_JIT_CODE_SIGN
#include "src/codegen/arm64/jit-code-signer-helper.h"
#include "src/codegen/arm64/jit-code-signer-hybrid.h"
#endif

namespace v8::internal::wasm {

class CachedAssemblerBuffer final : public AssemblerBuffer {
 public:
#ifdef V8_ENABLE_JIT_CODE_SIGN
  CachedAssemblerBuffer(AssemblerBufferCache* cache, base::AddressRegion region,
    std::unique_ptr<JitCodeSignerBase> signer = nullptr)
      : cache_(cache), region_(region), jit_code_signer_(std::move(signer)) {
    if (IsSupportJitCodeSigner() && jit_code_signer_ == nullptr) {
      jit_code_signer_ = std::make_unique<JitCodeSignerHybrid>();
    }
    TryRegisterTmpBuffer(jit_code_signer_.get(), reinterpret_cast<void *>(start()));
  }
#else
  CachedAssemblerBuffer(AssemblerBufferCache* cache, base::AddressRegion region)
      : cache_(cache), region_(region) {}
#endif

  ~CachedAssemblerBuffer() override { cache_->Return(region_); }

  uint8_t* start() const override {
    return reinterpret_cast<uint8_t*>(region_.begin());
  }

  int size() const override { return static_cast<int>(region_.size()); }

  std::unique_ptr<AssemblerBuffer> Grow(int new_size) override {
#ifdef V8_ENABLE_JIT_CODE_SIGN
    // remain this->jit_code_signer
    return cache_->GetAssemblerBuffer(new_size, this);
#else
    return cache_->GetAssemblerBuffer(new_size);
#endif
  }

#ifdef V8_ENABLE_JIT_CODE_SIGN
  JitCodeSignerBase *GetJitCodeSigner() const override {
    return jit_code_signer_.get();
  }
#endif

 private:
  AssemblerBufferCache* const cache_;
  const base::AddressRegion region_;

#ifdef V8_ENABLE_JIT_CODE_SIGN
  std::unique_ptr<JitCodeSignerBase> jit_code_signer_ = nullptr;
  friend class AssemblerBufferCache;
#endif
};

AssemblerBufferCache::~AssemblerBufferCache() {
  for (base::AddressRegion region : available_memory_.regions()) {
    GetWasmCodeManager()->FreeAssemblerBufferSpace(region);
  }
}

#ifdef V8_ENABLE_JIT_CODE_SIGN
std::unique_ptr<AssemblerBuffer> AssemblerBufferCache::GetAssemblerBuffer(
    int size, CachedAssemblerBuffer *buffer) {
#else
std::unique_ptr<AssemblerBuffer> AssemblerBufferCache::GetAssemblerBuffer(
    int size) {
#endif
  DCHECK_LT(0, size);
  base::AddressRegion region = available_memory_.Allocate(size);
  if (region.is_empty()) {
    static constexpr int kMinimumReservation = 64 * KB;
    int minimum_allocation =
        std::max(kMinimumReservation, std::max(total_allocated_ / 4, size));
    base::AddressRegion new_space =
        GetWasmCodeManager()->AllocateAssemblerBufferSpace(minimum_allocation);
    available_memory_.Merge(new_space);
    CHECK_GE(kMaxInt - total_allocated_, new_space.size());
    total_allocated_ += new_space.size();

    region = available_memory_.Allocate(size);
    DCHECK(!region.is_empty());
  }
#ifdef V8_ENABLE_JIT_CODE_SIGN
  return std::make_unique<CachedAssemblerBuffer>(this, region,
    buffer ? std::move(buffer->jit_code_signer_) : nullptr);
#else
  return std::make_unique<CachedAssemblerBuffer>(this, region);
#endif
}

void AssemblerBufferCache::Return(base::AddressRegion region) {
  available_memory_.Merge(region);
}

}  // namespace v8::internal::wasm
