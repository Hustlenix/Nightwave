#pragma once
// Host-only equivalent fixed-point primitives. Upstream assembly.h only
// supports embedded/MSVC-x86 targets. Do not spoof an embedded CPU macro.
#define _ASSEMBLY_H
#include <stdint.h>
typedef int64_t Word64;
static inline int MULSHIFT32(int x, int y) { return (int)(((int64_t)x * y) >> 32); }
static inline int FASTABS(int x) {
    const uint32_t sign = (uint32_t)(x >> 31);
    return (int)(((uint32_t)x ^ sign) - sign);
}
static inline int CLZ(int x) { return x ? __builtin_clz((uint32_t)x) : 32; }
static inline Word64 MADD64(Word64 sum, int x, int y) {
    return (Word64)((uint64_t)sum + (uint64_t)((int64_t)x * y));
}
static inline Word64 SHL64(Word64 x, int n) { return (Word64)((uint64_t)x << n); }
static inline Word64 SAR64(Word64 x, int n) { return x >> n; }
