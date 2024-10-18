#include "commands.h"

#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdlib>

namespace ae {

command_handler *command_handler::instance = nullptr;

command_handler & command_handler::get() {
    assert(instance);
    return *instance;
}

void command_handler::create(std::span<char *> arguments) {
    assert(!instance);
    instance = new command_handler(arguments);
}

void command_handler::destroy() {
    delete instance;
}

command_handler::variant command_handler::value(const std::string &argument) {
    if(!arguments_.contains(argument)) {
        return command_handler::variant{};
    }

    return arguments_[argument];
}

command_handler::command_handler(std::span<char *> arguments) {
    constexpr struct {
        u32 operand_offset;
        const char *cmd;
        const char *key;
        bool (command_handler::*parse)(const char *, command_handler::variant &);
    } table[] = {
        { 1, "--width", "width", &command_handler::parse_u32 },
        { 1, "--height", "height", &command_handler::parse_u32 },
        { 0, "--compute", "compute", &command_handler::parse_bool },
        { 1, "--output", "output", &command_handler::parse_str }
    };

    u32 set_args = 0;
    static_assert((sizeof(set_args) * 8) > AE_ARRAY_COUNT(table));

    for(u32 i = 0; i < arguments.size(); i++) {
        for(u32 j = 0; j < AE_ARRAY_COUNT(table); j++) {
            command_handler::variant var;

            if(!(set_args & (1 << j))
               && ((i + table[j].operand_offset) < arguments.size())
               && std::strcmp(arguments[i], table[j].cmd) == 0
               && (this->*table[j].parse)(arguments[i + table[j].operand_offset], var)) {

                arguments_.insert({table[j].key, var});

                i += table[j].operand_offset;
                set_args |= (1 << j);

                break;
            }
        }
    }

    // TODO: Verify if the set values are valid, and remove arguments that are mutually-exclusive
}

bool command_handler::parse_u32(const char *str, variant &var) {
    char *endpoint;
    u32 result = std::strtoul(str, &endpoint, 10);

    if(errno == ERANGE || result == ULONG_MAX) {
        var = {};
        return false;
    }

    if(*endpoint != '\0') {
        var = {};
        return false;
    }

    var = result;
    return true;
}

bool command_handler::parse_bool(const char *, variant &var) {
    // strcmp in the ctor already verifies that this argument is defined
    var = true;
    return true;
}

bool command_handler::parse_str(const char *str, variant &var) {
    var = std::string(str);
    return true;
}

}
