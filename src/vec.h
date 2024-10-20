#pragma once

#include "common.h"

#include <cassert>
#include <cmath>

namespace ae {
    template<typename TType>
    class vec4 {
    public:
        vec4()
            : x_(static_cast<TType>(0))
            , y_(static_cast<TType>(0))
            , z_(static_cast<TType>(0))
            , w_(static_cast<TType>(0)) {}

        vec4(TType val)
            : x_(val)
            , y_(val)
            , z_(val)
            , w_(val) {}

        vec4(TType x, TType y, TType z, TType w = static_cast<TType>(0))
            : x_(x)
            , y_(y)
            , z_(z)
            , w_(w) {}

        AE_FORCEINLINE TType x() const { return x_; }
        AE_FORCEINLINE TType y() const { return y_; }
        AE_FORCEINLINE TType z() const { return z_; }
        AE_FORCEINLINE TType w() const { return w_; }

        AE_FORCEINLINE void x(TType x) { x_ = x; }
        AE_FORCEINLINE void y(TType y) { y_ = y; }
        AE_FORCEINLINE void z(TType z) { z_ = z; }
        AE_FORCEINLINE void w(TType w) { w_ = w; }

        AE_FORCEINLINE vec4<TType> & operator+=(const vec4<TType> &other) {
            x_ += other.x();
            y_ += other.y();
            z_ += other.z();
            w_ += other.w();
            return *this;
        }

        AE_FORCEINLINE vec4<TType> & operator-=(const vec4<TType> &other) {
            x_ -= other.x();
            y_ -= other.y();
            z_ -= other.z();
            w_ -= other.w();
            return *this;
        }

        AE_FORCEINLINE vec4<TType> & operator*=(const vec4<TType> &other) {
            x_ *= other.x();
            y_ *= other.y();
            z_ *= other.z();
            w_ *= other.w();
            return *this;
        }

        AE_FORCEINLINE vec4<TType> & operator*=(TType s) {
            x_ *= s;
            y_ *= s;
            z_ *= s;
            w_ *= s;
            return *this;
        }

        AE_FORCEINLINE vec4<TType> & operator/=(const vec4<TType> &other) {
            x_ /= other.x();
            y_ /= other.y();
            z_ /= other.z();
            w_ /= other.w();
            return *this;
        }

        AE_FORCEINLINE vec4<TType> & operator/=(TType s) {
            x_ /= s;
            y_ /= s;
            z_ /= s;
            w_ /= s;
            return *this;
        }

        AE_FORCEINLINE vec4<TType> operator+(const vec4<TType> &other) const {
            vec4<TType> v = *this;
            v += other;
            return v;
        }

        AE_FORCEINLINE vec4<TType> operator-(const vec4<TType> &other) const {
            vec4<TType> v = *this;
            v -= other;
            return v;
        }

        AE_FORCEINLINE vec4<TType> operator*(const vec4<TType> &other) const {
            vec4<TType> v = *this;
            v *= other;
            return v;
        }

        AE_FORCEINLINE vec4<TType> operator*(TType s) const {
            vec4<TType> v = *this;
            v *= s;
            return v;
        }

        AE_FORCEINLINE vec4<TType> operator/(const vec4<TType> &other) const {
            vec4<TType> v = *this;
            v /= other;
            return v;
        }

        AE_FORCEINLINE vec4<TType> operator/(TType s) const {
            vec4<TType> v = *this;
            v /= s;
            return v;
        }

        AE_FORCEINLINE vec4<TType> operator-() { return vec4{-x(), -y(), -z(), -w()}; }

        void swizzle(char comp0, char comp1, char comp2, char comp3) {
            char components[] = { comp0, comp1, comp2, comp3 };
            ae::vec4<TType> v = *this;

            for(u32 i = 0; i < 4; i++) {
                switch(components[i] - 'w') {
                    case 0: v_[i] = v.w(); break;
                    case 1: v_[i] = v.x(); break;
                    case 2: v_[i] = v.y(); break;
                    case 3: v_[i] = v.z(); break;
                    default: assert(0); break;
                }
            }
        }

        AE_FORCEINLINE TType sum() const { return x_ + y_ + z_ + w_; }

        AE_FORCEINLINE TType dot(const vec4<TType> &other) const {
            return ((*this) * other).sum();
        }

        AE_FORCEINLINE TType magnitude_squared() const {
            const vec4<TType> &v = *this;
            return (v * v).sum();
        }

        AE_FORCEINLINE TType magnitude() const {
            TType msquared = magnitude_squared();

            if constexpr(sizeof(TType) == 4) {
                return std::sqrtf(msquared);
            } else {
                return static_cast<TType>(std::sqrt(msquared));
            }
        }

        AE_FORCEINLINE void normalize() {
            TType n = magnitude();
            if(n != static_cast<TType>(0)) [[likely]] {
                (*this) /= n;
            }
        }

        AE_FORCEINLINE vec4<TType> get_normalized() const {
            vec4<TType> v = *this;
            v.normalize();
            return v;
        }

    private:
        union {
            struct {
                TType x_, y_, z_, w_;
            };
            TType v_[4];
        };
    };

    template<typename TType>
    AE_FORCEINLINE vec4<TType> operator*(TType s, const ae::vec4<TType> &vec) {
        vec4<TType> v = vec;
        v *= s;
        return v;
    }

    using vec4f = vec4<f32>;
}
