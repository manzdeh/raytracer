// Compile using "dxc -spirv -T cs_6_0 -E main src/compute.hlsl -Fo ./compute.spv"

// NOTE: Temporary test compute shader to verify the pipeline

[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    RWTexture2D<float4> output;
    output[id.xy] = float4(1.0, 0.0, 0.0, 1.0);
}
