#pragma once

#include "common.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <utility>

namespace ae {
    template<typename TType>
    static AE_FORCEINLINE TType abs(TType value) { return (value >= static_cast<TType>(0)) ? value : -value; }

    template<typename TType>
    static AE_FORCEINLINE TType min(TType a, TType b) { return (a <= b) ? a : b; }

    template<typename TType>
    static AE_FORCEINLINE TType max(TType a, TType b) { return (a >= b) ? a : b; }

    template<typename TType>
    static AE_FORCEINLINE TType clamp(TType v, TType m0, TType m1) { return max(min(v, m1), m0); }

    template<typename TType>
    static AE_FORCEINLINE TType lerp(TType t, TType v0, TType v1) {
        return (static_cast<TType>(1) - t) * v0 + (t * v1);
    }

    template<typename TType>
    static AE_FORCEINLINE TType saturate(TType t) {
        return clamp(t, static_cast<TType>(0), static_cast<TType>(1));
    }

    template<typename TType>
    static AE_FORCEINLINE TType remap(TType value, std::pair<TType, TType> input_range, std::pair<TType, TType> output_range) {
        assert((input_range.second - input_range.first) > static_cast<TType>(0) && "Invalid or swapped input range provided");
        assert((output_range.second - output_range.first) > static_cast<TType>(0) && "Invalid or swapped output range provided");

        return output_range.first +
                (value - input_range.first) *
                (output_range.second - output_range.first) /
                (input_range.second - input_range.first);
    }

    static AE_FORCEINLINE bool nearly_equal(f32 a, f32 b, f32 epsilon = std::numeric_limits<f32>::epsilon()) {
        return std::fabs(a - b) < epsilon;
    }
}
