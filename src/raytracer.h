#pragma once

#include "common.h"

#include <span>

namespace ae {
    class raytracer {
    public:
        virtual ~raytracer() = default;

        virtual bool setup(u32 width, u32 height) = 0;
        virtual void trace() = 0;
        virtual std::span<u32> get_result() const = 0;

    protected:
        u32 width_ = 0;
        u32 height_ = 0;
    };
}
