#include "commands.h"

int main(int argc, char **argv) {
    ae::command_handler::create(argc, argv);

    ae::command_handler::destroy();

    return 0;
}
