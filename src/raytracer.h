#pragma once

#include "common.h"

#include <span>
#include <utility>

namespace ae {
    class raytracer {
    public:
        virtual ~raytracer() = default;

        virtual bool setup() = 0;
        virtual void trace() = 0;
        virtual std::span<u32> get_result() const = 0;

        static std::pair<u32, u32> get_resolution();

    protected:
        u32 width_ = 0;
        u32 height_ = 0;
    };
}
