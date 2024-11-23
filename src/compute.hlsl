// Compile using "dxc -spirv -T cs_6_0 -E main src/compute.hlsl -Fo ./compute.spv"

struct scene_constants {
    float4 background0;
    float4 background1;
    float4 cam;
    float4 sphere;
};

[[vk::push_constant]] scene_constants scene;
[[vk::binding(0, 0)]] RWTexture2D<float4> image;

bool has_intersection(in float4 dir) {
    float3 oc = scene.sphere.xyz - scene.cam.xyz;
    float a = dot(dir.xyz, dir.xyz);
    float b = -2.0 * dot(dir.xyz, oc);
    float c = dot(oc, oc) - scene.sphere.w * scene.sphere.w;

    float discriminant = b * b - 4 * a * c;

    if(discriminant >= 0) {
        return true;
    }

    return false;
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

    float4 viewport = float4(w, h, 0.0, 0.0);
    float4 pixel_size = viewport / float4(width, height, 1.0, 1.0);

    float4 uv = (float4(float2(id.xy) + float2(0.5, 0.5), 0.0, 0.0) * pixel_size)
        - (viewport * float4(0.5, 0.5, 1.0, 0.0));

    float4 dir = normalize(uv - scene.cam);

    if(has_intersection(dir)) {
        image[id.xy] = float4(1.0, 0.0, 0.0, 1.0);
    } else {
        image[id.xy] = float4(lerp(scene.background0.gba,
                                   scene.background1.gba,
                                   float(id.y) / height), 1.0);
    }
}
