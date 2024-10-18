#include "commands.h"
#include "software_raytracer.h"
#include "vulkan_raytracer.h"

#include <memory>
#include <utility>

#include <string_view>

static void run_raytracer();

int main(int argc, char *argv[]) {
    ae::command_handler::create(std::span(argv, argc));
    run_raytracer();
    ae::command_handler::destroy();

    return 0;
}

void run_raytracer() {
    ae::command_handler &commands = ae::command_handler::get();

    auto get_resolution = [&commands]() -> std::pair<u32, u32> {
        u32 width = 0, height = 0; // TODO: Should setup or query default values

        ae::command_handler::variant variant;
        if(variant = commands.value("width"); std::holds_alternative<u32>(variant)) {
            width = std::get<u32>(variant);
        }

        if(variant = commands.value("height"); std::holds_alternative<u32>(variant)) {
            height = std::get<u32>(variant);
        }

        return std::make_pair(width, height);
    };

    std::unique_ptr<ae::raytracer> raytracer;

    // TODO: Handle return value
    auto create_software_raytracer = [&get_resolution, &raytracer]() {
        raytracer = std::make_unique<ae::software_raytracer>();
        auto [w, h] = get_resolution();
        return raytracer->setup(w, h);
    };

    ae::command_handler::variant variant;

    if(variant = commands.value("compute"); std::holds_alternative<bool>(variant)) {
        raytracer = std::make_unique<ae::vulkan_raytracer>();
        auto [w, h] = get_resolution();

        if(!raytracer->setup(w, h)) {
            create_software_raytracer();
        }
    } else {
        create_software_raytracer();
    }
}
