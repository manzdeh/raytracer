#pragma once

#include <cstdint>
#include <cstddef>

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using f32 = float;
using f64 = double;

#define AE_ARRAY_COUNT(array) (sizeof(array) / (sizeof((array)[0])))

#ifdef AE_PLATFORM_WIN32
    #define AE_FORCEINLINE __forceinline
#else
    #define AE_FORCEINLINE inline
#endif
