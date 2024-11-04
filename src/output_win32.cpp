#include "output.h"

#include "common_win32.h"

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

        const size_t size = file_size();

        file_data->mapping_ = CreateFileMappingA(handle,
                                                 nullptr,
                                                 PAGE_READWRITE,
                                                 static_cast<DWORD>(size >> 32),
                                                 static_cast<DWORD>(size & 0xffffffff),
                                                 nullptr);

        if(file_data->mapping_ != INVALID_HANDLE_VALUE) {
            file_data->view_ = MapViewOfFile(file_data->mapping_,
                                             FILE_MAP_ALL_ACCESS,
                                             0,
                                             0,
                                             size);

            if(file_data->view_) {
                write_default_header(file_data->view_);
            }
        }

        impl_ = file_data;
    }
}

output::~output() {
    if(impl_) {
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
    }
}

void * output::get_buffer() {
    if(impl_) {
        win32_mapping_data *data = reinterpret_cast<win32_mapping_data *>(impl_);
        return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(data->view_)
                                        + sizeof(tga_file_header));
    }

    return nullptr;
}

}
