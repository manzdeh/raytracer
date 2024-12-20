#include "output.h"

#include "common_linux.h"

namespace ae {

struct linux_memory_mapped_file {
    void *mapping_ = nullptr;
    int fd_ = -1;
};

output::output(std::string_view file_name) {
    int fd = open(file_name.data(),
                  O_CREAT | O_TRUNC | O_RDWR,
                  0644);

    if(fd != -1) {
        linux_memory_mapped_file *memory_mapped_file = new linux_memory_mapped_file();
        memory_mapped_file->fd_ = fd;

        const size_t size = file_size();
        ftruncate(memory_mapped_file->fd_, size);

        memory_mapped_file->mapping_ = mmap(nullptr,
                                            size,
                                            PROT_READ | PROT_WRITE,
                                            MAP_SHARED,
                                            memory_mapped_file->fd_,
                                            0);

        if(memory_mapped_file->mapping_) {
            write_default_header(memory_mapped_file->mapping_);
        }

        impl_ = memory_mapped_file;
    }
}

output::~output() {
    if(impl_) {
        linux_memory_mapped_file *memory_mapped_file = reinterpret_cast<linux_memory_mapped_file *>(impl_);

        if(memory_mapped_file->mapping_) {
            munmap(memory_mapped_file->mapping_, file_size());
        }

        if(memory_mapped_file->fd_ != -1) {
            close(memory_mapped_file->fd_);
        }

        delete memory_mapped_file;
    }
}

void * output::get_buffer() {
    if(impl_) {
        linux_memory_mapped_file *memory_mapped_file = reinterpret_cast<linux_memory_mapped_file *>(impl_);
        return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(memory_mapped_file->mapping_)
                                        + sizeof(tga_file_header));
    }

    return nullptr;
}

}
