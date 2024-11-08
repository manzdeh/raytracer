#include "commands.h"
#include "output.h"
#include "software_raytracer.h"
#include "system.h"
#include "vulkan_raytracer.h"

#include <memory>

static void run_raytracer(void *buffer);

int main(int argc, char *argv[]) {
    ae::system_init();

    ae::command_handler::create(std::span(argv, argc));

    std::unique_ptr<ae::output> output =
        std::make_unique<ae::output>(std::string_view("output.tga"));

    run_raytracer(output->get_buffer());
    ae::command_handler::destroy();

    return 0;
}

void run_raytracer(void *buffer) {
    std::unique_ptr<ae::raytracer> raytracer;

    auto create_software_raytracer = [&raytracer, buffer]() {
        raytracer = std::make_unique<ae::software_raytracer>(reinterpret_cast<u32 *>(buffer));
        return raytracer->setup();
    };

    ae::command_handler::variant variant;
    bool success = false;

    const ae::command_handler &cmdhandler = ae::command_handler::get();

    if(cmdhandler.has("compute"_hash)
       && std::get<bool>(cmdhandler.value("compute"_hash))
       && ae::vulkan_raytracer::init()) {

        raytracer = std::make_unique<ae::vulkan_raytracer>(reinterpret_cast<u32 *>(buffer));
        success = raytracer->setup();

        if(!success) {
            success = create_software_raytracer();
        }
    } else {
        success = create_software_raytracer();
    }

    if(success) {
        raytracer->trace();
        ae::vulkan_raytracer::terminate();
    } else {
        // TODO: Print an error message to stderr
    }
}
