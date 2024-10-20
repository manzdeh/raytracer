#include "output.h"

namespace ae {

output::output(std::string_view /*file_name*/) {
}

output::~output() {
    impl_ = nullptr;
}

void * output::get_buffer() {
    return nullptr;
}

}
