#pragma once

#include "common.h"

namespace ae {
    using strhash = u64;
}

consteval ae::strhash fnv1a64_str_hash(const char *str) {
    ae::strhash hash = 0xcbf29ce484222325ull;

    while(*str) {
        hash ^= *str++;
        hash *= 0x100000001b3;
    }

    return hash;
}

consteval ae::strhash operator""_hash(const char *str, size_t) {
    return fnv1a64_str_hash(str);
}
