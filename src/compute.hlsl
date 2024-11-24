// Compile using "dxc -spirv -T cs_6_0 -E main src/compute.hlsl -Fo ./compute.spv"

struct scene_constants {
    float4 background0;
    float4 background1;
    float4 cam;
    float4 sphere;
};

[[vk::push_constant]] scene_constants scene;
[[vk::binding(0, 0)]] RWTexture2D<float4> image;

struct hit_info {
    float3 pos;
    float3 normal;
    float t;
};

bool has_intersection(in float3 origin, in float3 dir, out hit_info info) {
    float radius = scene.sphere.w;

    float3 oc = scene.sphere.xyz - origin;
    float a = dot(dir, dir);
    float b = -2.0 * dot(dir, oc);
    float c = dot(oc, oc) - radius * radius;

    float discriminant = b * b - 4 * a * c;

    if(discriminant >= 0) {
        info.t = (-b - sqrt(discriminant)) / (2.0 * a);
        info.pos = origin + (dir * info.t);
        info.normal = normalize(info.pos - scene.sphere.xyz);
        return true;
    }

    return false;
}

float3 remap(in float3 value,
             in float input_min,
             in float input_max,
             in float output_min,
             in float output_max) {

    float3 i0 = (float3)input_min;
    float3 i1 = (float3)input_max;
    float3 o0 = (float3)output_min;
    float3 o1 = (float3)output_max;

    return o0
        + (value - i0)
        * (o1 - o0)
        / (i1 - i0);
}

[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    float width, height;
    image.GetDimensions(width, height);

    // TODO: This can be done once on program startup instead
    float w, h;
    if(width > height) {
        w = width / height;
        h = 1.0;
    } else {
        w = 1.0;
        h = height / width;
    }

    float3 viewport = float3(w, h, 0);
    float3 pixel_size = viewport / float3(width, height, 1.0);

    float3 uv = (float3(float2(id.xy) + float2(0.5, 0.5), 0) * pixel_size)
        - (viewport * float3(0.5, 0.5, 1.0));

    float3 origin = scene.cam.xyz;
    float3 dir = normalize(uv - origin);

    hit_info info;
    if(has_intersection(origin, dir, info)) {
        image[id.xy] = float4(remap(info.normal, -1.0, 1.0, 0.0, 1.0), 1.0);
    } else {
        image[id.xy] = float4(lerp(scene.background0.gba,
                                   scene.background1.gba,
                                   float(id.y) / height), 1.0);
    }
}
