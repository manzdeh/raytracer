#include "raytracer.h"

#include "commands.h"

bool ae::raytracer::query_resolution() {
    ae::command_handler &commands = ae::command_handler::get();

    ae::command_handler::variant width = commands.value("width"_hash);
    ae::command_handler::variant height = commands.value("height"_hash);

    if(std::holds_alternative<u32>(width) && std::holds_alternative<u32>(height)) {
        width_ = std::get<u32>(width);
        height_ = std::get<u32>(height);
        return true;
    }

    return false;
}
