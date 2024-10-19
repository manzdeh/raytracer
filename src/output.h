#pragma once

#include "common.h"

#include <string_view>

namespace ae {
    class output {
    public:
        output(std::string_view file_name);
        ~output();

        void * get_buffer();

    private:
#pragma pack(push, 1)
        struct tga_file_header {
            u8 id_length_ = 0;
            u8 color_map_type_ = 0;
            u8 image_type_ = 0;
            u16 cmap_first_index_ = 0;
            u16 cmap_length_ = 0;
            u8 cmap_entry_size_ = 0;
            u16 x_origin_ = 0;
            u16 y_origin_ = 0;
            u16 width_ = 0;
            u16 height_ = 0;
            u8 pixel_depth_ = 0;
            u8 img_descriptor_ = 0;
        };
#pragma pack(pop)

        void *impl_ = nullptr;
    };
}
