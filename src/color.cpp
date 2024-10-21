#include "color.h"

#include "math.h"

namespace ae {

hsv hsv::lerp(const hsv &other, f32 t) const {
    return {
        .h_ = ae::lerp(h_ , other.h_, t),
        .s_ = ae::lerp(s_ , other.s_, t),
        .v_ = ae::lerp(v_ , other.v_, t),
    };
}

color::color(f32 r, f32 g, f32 b, f32 a)
    : a_(a)
    , r_(r)
    , g_(g)
    , b_(b) {}

color::color(u32 argb)
    : a_(static_cast<f32>(argb >> 24) / 255.0f)
    , r_(static_cast<f32>((argb >> 16) & 0xff) / 255.0f)
    , g_(static_cast<f32>((argb >> 8) & 0xff) / 255.0f)
    , b_(static_cast<f32>(argb & 0xff) / 255.0f) {}

color::color(const hsv &hsv_color) {
    if(hsv_color.s_ == 0.0f) {
        r_ = hsv_color.v_;
        g_ = hsv_color.v_;
        b_ = hsv_color.v_;
        return;
    }

    f32 hsv_h = hsv_color.h_ / 60.0f;

    const u32 sector = static_cast<u32>(hsv_h);
    const f32 f = hsv_h - sector;

#define HSV_P hsv_color.v_ * (1.0f - hsv_color.s_)
#define HSV_Q hsv_color.v_ * (1.0f - hsv_color.s_ * f)
#define HSV_T hsv_color.v_ * (1.0f - hsv_color.s_ * (1.0f - f))

    switch(sector) {
        case 0:
            r_ = hsv_color.v_;
            g_ = HSV_T;
            b_ = HSV_P;
            break;
        case 1:
            r_ = HSV_Q;
            g_ = hsv_color.v_;
            b_ = HSV_P;
            break;
        case 2:
            r_ = HSV_P;
            g_ = hsv_color.v_;
            b_ = HSV_T;
            break;
        case 3:
            r_ = HSV_P;
            g_ = HSV_Q;
            b_ = hsv_color.v_;
            break;
        case 4:
            r_ = HSV_T;
            g_ = HSV_P;
            b_ = hsv_color.v_;
            break;
        case 5:
        default:
            r_ = hsv_color.v_;
            g_ = HSV_P;
            b_ = HSV_Q;
            break;
    }

#undef HSV_T
#undef HSV_Q
#undef HSV_P
}

u32 color::get_argb32() const {
    // TODO: Need to check and measure if this is faster when using SSE
    // (might be slower due to data conversions between vector registers and general-purpose ones)

    return
        (static_cast<u32>(a_ * 255.0f) << 24)
        | (static_cast<u32>(r_ * 255.0f) << 16)
        | (static_cast<u32>(g_ * 255.0f) << 8)
        | (static_cast<u32>(b_ * 255.0f) & 0xff);
}

hsv color::to_hsv() const {
    f32 min = ae::min(r_, ae::min(g_, b_));
    f32 max = ae::max(r_, ae::max(g_, b_));

    f32 diff = max - min;

    if(diff == 0.0f) {
        return { .v_ = max };
    }

    f32 h = 0.0f;
    if(max == r_) {
        h = (g_ - b_) / diff;
    } else if(max == g_) {
        h = ((b_ - r_) / diff) + 2.0f;
    } else if(max == b_) {
        h = ((r_ - g_) / diff) + 4.0f;
    }

    h *= 60.0f;
    if(h < 0.0f) {
        h += 360.0f;
    }

    return {
        .h_ = h,
        .s_ = diff / max,
        .v_ = max
    };
}

}
