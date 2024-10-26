#pragma once

#include "common.h"

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

}
