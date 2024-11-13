#pragma once

#include "common.h"

namespace ae {
    struct hsv {
        hsv lerp(const hsv &other, f32 t) const;

        f32 h_;
        f32 s_;
        f32 v_;
        f32 a_;
    };

    class color {
    public:
        color() = default;
        color(u32 argb);
        color(f32 r, f32 g, f32 b, f32 a = 1.0f);
        color(const hsv &hsv_color);

        u32 get_argb32() const;

        hsv to_hsv() const;

        f32 a_ = 0.0f;
        f32 r_ = 0.0f;
        f32 g_ = 0.0f;
        f32 b_ = 0.0f;
    };
}

#define AE_ARGB(a, r, g, b) \
    static_cast<u32>((((a) << 24) \
                      | ((r) << 16) \
                      | ((g) << 8) \
                      | ((b))))

#define AE_RGB(r, g, b) AE_ARGB(0xff, r, g, b)
