#include "commands.h"
#include "software_raytracer.h"
#include "vulkan_raytracer.h"

#include <memory>
#include <utility>

static void run_raytracer();

int main(int argc, char *argv[]) {
    ae::command_handler::create(std::span(argv, argc));
    run_raytracer();
    ae::command_handler::destroy();

    return 0;
}

void run_raytracer() {
    std::unique_ptr<ae::raytracer> raytracer;

    // TODO: Handle return value
    auto create_software_raytracer = [&raytracer]() {
        raytracer = std::make_unique<ae::software_raytracer>();
        return raytracer->setup();
    };

    ae::command_handler::variant variant;

    if(variant = ae::command_handler::get().value("compute"_hash); std::holds_alternative<bool>(variant)) {
        raytracer = std::make_unique<ae::vulkan_raytracer>();

        if(!raytracer->setup()) {
            create_software_raytracer();
        }
    } else {
        create_software_raytracer();
    }
}
