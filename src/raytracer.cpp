#include "raytracer.h"

#include "commands.h"

namespace {
    u32 raytracer_width = 0;
    u32 raytracer_height = 0;
}

std::pair<u32, u32> ae::raytracer::get_resolution() {
    if(raytracer_width > 0 && raytracer_height > 0) {
        return std::make_pair(raytracer_width, raytracer_height);
    }

    ae::command_handler &commands = ae::command_handler::get();

    ae::command_handler::variant width = commands.value("width"_hash);
    ae::command_handler::variant height = commands.value("height"_hash);

    if(std::holds_alternative<u32>(width) && std::holds_alternative<u32>(height)) {
        raytracer_width = std::get<u32>(width);
        raytracer_height = std::get<u32>(height);
    }

    return std::make_pair(raytracer_width, raytracer_height);
}
