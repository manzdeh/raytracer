#pragma once

#include "common.h"

#include <string>
#include <string_view>
#include <variant>
#include <unordered_map>

namespace ae {
    class command_handler {
        using key = std::string;

    public:
        using variant = std::variant<std::monostate, bool, u32, std::string>;

        static command_handler & get();

        static void create(int argc, char **argv);
        static void destroy();

        variant value(const std::string &argument);

    private:
        command_handler(int argc, char **argv);

        bool parse_u32(const char *str, variant &var);
        bool parse_bool(const char *str, variant &var);
        bool parse_str(const char *str, variant &var);

        static command_handler *instance;
        std::unordered_map<key, variant> arguments; // TODO: Change std::string to compile-time hashed value
    };
}
