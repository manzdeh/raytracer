#include "software_raytracer.h"

#include "color.h"
#include "math.h"
#include "ray.h"
#include "shapes.h"
#include "vec.h"

#include <utility>

// TODO: Temporary threaded test
#ifdef AE_PLATFORM_LINUX
#include "common_linux.h"
#include <pthread.h>

static pthread_t test_thread; // TODO: Not destroyed yet
static pthread_mutex_t test_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t test_condition_variable = PTHREAD_COND_INITIALIZER;
#endif

namespace ae {

// Test scene
static const ae::vec4f camera_pos(0.0f, 0.0f, 1.0f);
static const ae::sphere sphere(ae::vec4f(0.0f, 0.0f, -2.0f), 1.0f);

software_raytracer::software_raytracer(u32 *buffer)
    : framebuffer_(buffer) {
}

bool software_raytracer::setup() {
    auto [width, height] = raytracer::get_resolution();
    width_ = width;
    height_ = height;

    f32 w, h;
    if(width_ > height_) {
        w = static_cast<f32>(width_) / static_cast<f32>(height_);
        h = 1.0f;
    } else {
        w = 1.0f;
        h = static_cast<f32>(height_) / static_cast<f32>(width_);
    }

    viewport_size_ = ae::vec4f(w, h, 0.0f);
    pixel_size_ = ae::vec4f(viewport_size_.x_ / static_cast<f32>(width_),
                            viewport_size_.y_ / static_cast<f32>(height_),
                            0.0f);

    row_count_ = width_ / ae::raytracer::tile_size;
    col_count_ = height_ / ae::raytracer::tile_size;

    return true;
}

void software_raytracer::trace() {
    auto copy_to_framebuffer = [this](const tile_data &tile) {
        const u32 ystart = tile.col * ae::raytracer::tile_size;
        const u32 yend = ystart + ae::raytracer::tile_size;

        const u32 xstart = tile.row * ae::raytracer::tile_size;
        const u32 xend = xstart + ae::raytracer::tile_size;

        u32 tile_index = 0;

        for(u32 y = ystart; y < yend; y++) {
            for(u32 x = xstart; x < xend; x++) {
                framebuffer_[y * width_ + x] = tile.pixels[tile_index++];
            }
        }
    };

#ifdef AE_PLATFORM_LINUX
    // TODO: Not used yet, first a test with a single thread
    long thread_count = sysconf(_SC_NPROCESSORS_ONLN);
    if(thread_count != -1) {
        thread_count = ae::max<long>(0, thread_count - 1);
    }

    pthread_attr_t attrib;
    pthread_attr_init(&attrib);
    pthread_attr_setdetachstate(&attrib, PTHREAD_CREATE_DETACHED);

    pthread_create(&test_thread,
                   &attrib,
                   software_raytracer::thread_func,
                   this);

    pthread_attr_destroy(&attrib);
#endif

#if 0
    const u32 count_i = width_ / ae::raytracer::tile_size;
    const u32 count_j = height_ / ae::raytracer::tile_size;

    tile_data tile;

    for(u32 j = 0; j < count_j; j++) {
        tile.col = j;

        for(u32 i = 0; i < count_i; i++) {
            tile.row = i;
            trace_tile(tile);
            copy_to_framebuffer(tile);
        }
    }

#else

    for(;;) {
        pthread_mutex_lock(&test_mutex);
        while(tile_queue_.empty() && !finished_) {
            pthread_cond_wait(&test_condition_variable, &test_mutex);
        }

        if(tile_queue_.empty() && finished_) {
            pthread_mutex_unlock(&test_mutex);
            break;
        }

        copy_to_framebuffer(tile_queue_.front());
        tile_queue_.pop();

        pthread_mutex_unlock(&test_mutex);
    }

#endif
}

void software_raytracer::trace_tile(tile_data &tile) {
    const u32 xstart = tile.row * ae::raytracer::tile_size;
    const u32 ystart = tile.col * ae::raytracer::tile_size;

    for(u32 y = 0; y < ae::raytracer::tile_size; y++) {
        const f32 yf = static_cast<f32>(y + ystart);
        const f32 t = yf / static_cast<f32>(height_);

        const u32 background = ae::color(ae::lerp(t, ae::raytracer::background0.r_, ae::raytracer::background1.r_),
                                         ae::lerp(t, ae::raytracer::background0.g_, ae::raytracer::background1.g_),
                                         ae::lerp(t, ae::raytracer::background0.b_, ae::raytracer::background1.b_)).get_argb32();

        for(u32 x = 0; x < ae::raytracer::tile_size; x++) {
            const ae::vec4f uv = (ae::vec4f(static_cast<f32>(x + xstart) + 0.5f, yf + 0.5f, 0.0f) * pixel_size_)
                - (viewport_size_ * ae::vec4f(0.5f, 0.5f, 1.0f));

            const ae::ray ray(camera_pos, uv - camera_pos);
            ae::ray_hit_info hit_info;

            u32 *pixel = &tile.pixels[y * ae::raytracer::tile_size + x];

            if(sphere.intersects(ray, hit_info)) {
                const std::pair<f32, f32> input{-1.0f, 1.0f};
                const std::pair<f32, f32> output{0.0f, 1.0f};

                const ae::color c(ae::remap(hit_info.normal_.x_, input, output),
                                  ae::remap(hit_info.normal_.y_, input, output),
                                  ae::remap(hit_info.normal_.z_, input, output));

                *pixel = c.get_argb32();
            } else {
                *pixel = background;
            }
        }
    }
}

bool software_raytracer::get_next_tile(tile_data &tile) {
    if(current_col_ < col_count_) {
        tile.row = current_row_;
        tile.col = current_col_;

        current_row_++;

        if(current_row_ == row_count_) {
            current_row_ = 0;
            current_col_++;
        }

        return true;
    }

    finished_ = true;
    return false;
}

void * software_raytracer::thread_func(void *data) {
    software_raytracer *rt = static_cast<software_raytracer *>(data);

    tile_data tile;

    while(rt->get_next_tile(tile)) {
        rt->trace_tile(tile);

        pthread_mutex_lock(&test_mutex);

        rt->tile_queue_.push(std::move(tile));

        pthread_cond_signal(&test_condition_variable);
        pthread_mutex_unlock(&test_mutex);
    }

    return 0;
}

}
