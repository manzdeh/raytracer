// Compile using "dxc -spirv -T cs_6_0 -E main src/compute.hlsl -Fo ./compute.spv"

struct scene_constants {
    float4 background0;
    float4 background1;
};

[[vk::push_constant]] scene_constants scene;
[[vk::binding(0, 0)]] RWTexture2D<float4> image;

[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    float width, height;
    image.GetDimensions(width, height);

    float t = float(id.y) / height;

    image[id.xy] = float4(lerp(scene.background0.gba,
                               scene.background1.gba,
                               t), 1.0);
}
