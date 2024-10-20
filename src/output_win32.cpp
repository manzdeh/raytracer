#include "output.h"

#include "common_win32.h"
#include "raytracer.h"

#include <cstring>

struct win32_mapping_data {
    HANDLE handle_ = INVALID_HANDLE_VALUE;
    HANDLE mapping_ = INVALID_HANDLE_VALUE;
    LPVOID view_ = nullptr;
};

namespace ae {

output::output(std::string_view file_name) {
    HANDLE handle = CreateFileA(file_name.data(),
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ,
                                nullptr,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                nullptr);

    if(handle != INVALID_HANDLE_VALUE) {
        win32_mapping_data *file_data = new win32_mapping_data();
        file_data->handle_ = handle;

        auto [width, height] = raytracer::get_resolution();
        size_t file_size = sizeof(tga_file_header) + (width * height * sizeof(u32));

        file_data->mapping_ = CreateFileMappingA(handle,
                                                 nullptr,
                                                 PAGE_READWRITE,
                                                 static_cast<DWORD>(file_size >> 32),
                                                 static_cast<DWORD>(file_size & 0xffffffff),
                                                 nullptr);

        if(file_data->mapping_ != INVALID_HANDLE_VALUE) {
            file_data->view_ = MapViewOfFile(file_data->mapping_,
                                             FILE_MAP_ALL_ACCESS,
                                             0,
                                             0,
                                             file_size);

            if(file_data->view_) {
                tga_file_header header = {
                    .image_type_ = 2,
                    .width_ = static_cast<u16>(width),
                    .height_ = static_cast<u16>(height),
                    .pixel_depth_ = 32
                };

                std::memcpy(file_data->view_, &header, sizeof(header));
            }
        }

        impl_ = file_data;
    }
}

output::~output() {
    win32_mapping_data *file_data = reinterpret_cast<win32_mapping_data *>(impl_);

    if(file_data->view_) {
        UnmapViewOfFile(file_data->view_);
    }

    if(file_data->mapping_ != INVALID_HANDLE_VALUE) {
        CloseHandle(file_data->mapping_);
    }

    if(file_data->handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(file_data->handle_);
    }

    delete file_data;
    impl_ = nullptr;
}

void * output::get_buffer() {
    win32_mapping_data *data = reinterpret_cast<win32_mapping_data *>(impl_);
    return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(data->view_)
                                    + sizeof(tga_file_header));
}

}
