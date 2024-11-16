// Compile using "dxc -spirv -T cs_6_0 -E main src/compute.hlsl -Fo ./compute.spv"

[[vk::binding(0, 0)]] RWTexture2D<float4> image;

[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    image[id.xy] = float4(1.0, 0.0, 0.0, 1.0);
}
