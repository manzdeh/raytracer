#include "software_raytracer.h"

#include "aemath.h"
#include "color.h"
#include "ray.h"
#include "shapes.h"
#include "vec.h"

#include <utility>

template<typename TType, auto TLockFunc, auto TUnlockFunc>
class scoped_lock {
public:
    scoped_lock(TType mutex)
        : mutex_(mutex) {
        TLockFunc(mutex_);
    }

    ~scoped_lock() {
        TUnlockFunc(mutex_);
    }

private:
    TType mutex_;
};

#ifdef AE_PLATFORM_WIN32

#include "common_win32.h"

using ae_scoped_lock = scoped_lock<PCRITICAL_SECTION, EnterCriticalSection, LeaveCriticalSection>;
using ae_mutex = CRITICAL_SECTION;
using ae_condition_variable = CONDITION_VARIABLE;

#define AE_MUTEX_INITIALIZER {0}
#define AE_CONDITION_VARIABLE_INITIALIZER CONDITION_VARIABLE_INIT

#elif defined(AE_PLATFORM_LINUX)

#include "common_linux.h"
#include <pthread.h>

using ae_scoped_lock = scoped_lock<pthread_mutex_t *, pthread_mutex_lock, pthread_mutex_unlock>;
using ae_mutex = pthread_mutex_t;
using ae_condition_variable = pthread_cond_t;

#define AE_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#define AE_CONDITION_VARIABLE_INITIALIZER PTHREAD_COND_INITIALIZER

#endif

static ae_mutex queue_mutex = AE_MUTEX_INITIALIZER;
static ae_mutex next_tile_mutex = AE_MUTEX_INITIALIZER;
static ae_condition_variable queue_ready_cv = AE_CONDITION_VARIABLE_INITIALIZER;

static void ae_cond_wait(ae_condition_variable *cv, ae_mutex *mutex);
static void cond_signal(ae_condition_variable *cv);

#undef AE_CONDITION_VARIABLE_INITIALIZER
#undef AE_MUTEX_INITIALIZER

namespace ae {

// Test scene
static const ae::vec4f camera_pos(0.0f, 0.0f, 1.0f);
static const ae::sphere sphere(ae::vec4f(0.0f, 0.0f, -2.0f), 1.0f);

software_raytracer::software_raytracer(u32 *buffer)
    : raytracer(buffer) {
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

    auto single_threaded_routine = [this, &copy_to_framebuffer]() {
        tile_data tile;

        while(get_next_tile(tile)) {
            trace_tile(tile);
            copy_to_framebuffer(tile);
        }
    };

    i32 thread_count = 0;

#ifdef AE_PLATFORM_WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    std::vector<HANDLE> threads;

    if(sysinfo.dwNumberOfProcessors > 1) {
        InitializeCriticalSectionAndSpinCount(&queue_mutex, 4000);
        InitializeCriticalSectionAndSpinCount(&next_tile_mutex, 4000);

        thread_count = static_cast<i32>(sysinfo.dwNumberOfProcessors - 1);
        threads.reserve(thread_count);

        for(i32 i = 0; i < thread_count; i++) {
            threads.emplace_back(CreateThread(nullptr,
                                              0,
                                              software_raytracer::thread_func<DWORD>,
                                              this,
                                              0,
                                              nullptr));
        }
    }

#elif defined(AE_PLATFORM_LINUX)
    thread_count = static_cast<i32>(sysconf(_SC_NPROCESSORS_ONLN));

    if(thread_count != -1) {
        if(thread_count = ae::max<long>(0, thread_count - 1); thread_count > 0) {
            pthread_attr_t attrib;
            pthread_attr_init(&attrib);
            pthread_attr_setdetachstate(&attrib, PTHREAD_CREATE_DETACHED);

            for(u32 i = 0; i < thread_count; i++) {
                pthread_t thread;
                pthread_create(&thread,
                               &attrib,
                               software_raytracer::thread_func<void *>,
                               this);
            }

            pthread_attr_destroy(&attrib);
        }
    }
#endif

    if(thread_count > 0) {
        for(;;) {
            tile_data tile;

            {
                ae_scoped_lock lock{&queue_mutex};

                while(tile_queue_.empty() && !finished_) {
                    ae_cond_wait(&queue_ready_cv, &queue_mutex);
                }

                if(tile_queue_.empty() && finished_) {
                    break;
                }

                tile = tile_queue_.front();
                tile_queue_.pop();
            }

            copy_to_framebuffer(tile);
        }
    } else {
        single_threaded_routine();
    }

#ifdef AE_PLATFORM_WIN32
    if(!threads.empty()) {
        WaitForMultipleObjects(static_cast<DWORD>(threads.size()), threads.data(), TRUE, INFINITE);

        for(HANDLE h : threads) {
            CloseHandle(h);
        }

        DeleteCriticalSection(&next_tile_mutex);
        DeleteCriticalSection(&queue_mutex);
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
    ae_scoped_lock lock{&next_tile_mutex};

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

template<typename TType>
TType software_raytracer::thread_func(void *data) {
    software_raytracer *rt = static_cast<software_raytracer *>(data);

    tile_data tile;

    while(rt->get_next_tile(tile)) {
        rt->trace_tile(tile);

        ae_scoped_lock lock{&queue_mutex};
        rt->tile_queue_.push(tile);
        cond_signal(&queue_ready_cv);
    }

    return static_cast<TType>(0);
}

}

#ifdef AE_PLATFORM_WIN32
void ae_cond_wait(ae_condition_variable *cv, ae_mutex *mutex) { SleepConditionVariableCS(cv, mutex, INFINITE); }
void cond_signal(ae_condition_variable *cv) { WakeConditionVariable(cv); }
#elif defined(AE_PLATFORM_LINUX)
void ae_cond_wait(ae_condition_variable *cv, ae_mutex *mutex) { pthread_cond_wait(cv, mutex); }
void cond_signal(ae_condition_variable *cv) { pthread_cond_signal(cv); }
#endif
