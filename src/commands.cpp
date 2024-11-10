#include "commands.h"

#include "aemath.h"
#include "raytracer.h"

#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <optional>

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

bool command_handler::has(ae::strhash argument) const {
    return arguments_.contains(argument);
}

command_handler::variant command_handler::value(ae::strhash argument) const {
    return has(argument) ? arguments_.at(argument) : command_handler::variant{};
}

command_handler::command_handler(std::span<char *> arguments) {
    constexpr struct {
        u32 operand_offset;
        const char *cmd;
        ae::strhash key;
        bool (command_handler::*parse)(const char *, command_handler::variant &);
        std::optional<ae::command_handler::variant> default_value = std::nullopt;
    } table[] = {
        { 1, "--width", "width"_hash, &command_handler::parse_u32, 512u },
        { 1, "--height", "height"_hash, &command_handler::parse_u32, 512u },
        { 0, "--compute", "compute"_hash, &command_handler::parse_bool, false },
        { 1, "--output", "output"_hash, &command_handler::parse_str } // TODO: Not used currently. Will be used to write out the rendered image to a path
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

    // Set default values if they were not specified
    for(u32 i = 0; i < AE_ARRAY_COUNT(table); i++) {
        if(!arguments_.contains(table[i].key)
           && table[i].default_value.has_value()) {
            arguments_.insert({table[i].key, *table[i].default_value});
        }
    }

    auto align_size = [](command_handler::variant &val) {
        const u32 alignment = ae::raytracer::tile_size;
        u32 current = std::get<u32>(val);

        val = ae::max(static_cast<u32>((current + (alignment - 1)) & ~(alignment - 1)),
                      alignment);
    };

    align_size(arguments_.at("width"_hash));
    align_size(arguments_.at("height"_hash));
}

bool command_handler::parse_u32(const char *str, variant &var) {
    char *endpoint;
    unsigned long result = std::strtoul(str, &endpoint, 10);

    if(errno == ERANGE || result == ULONG_MAX) {
        var = {};
        return false;
    }

    if(*endpoint != '\0') {
        var = {};
        return false;
    }

    var = static_cast<u32>(result);
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
