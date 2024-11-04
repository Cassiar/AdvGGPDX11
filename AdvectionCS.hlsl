#include "FluidSimHelpers.hlsli"

// Defines the input to this compute shader
// used to handle globabl variables
cbuffer ExternalData : register(b0) {
	float deltaTime;
	int gridRes; 
};

RWTexture3D<float4> UavOutputMap : register (u0);
Texture3D<float4> InputMap : register (t0);
Texture3D<float4> VelocityMap : register (t1);
//Texture3D<float4> DensityMap : register (t2);

SamplerState LinearClampSampler : register(s0);

[numthreads(GROUP_SIZE, GROUP_SIZE, GROUP_SIZE)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	//we're using center of each cell for sampling
	//get coord from thread id 
	//float3 coords = (float3(DTid)+0.5f) * invFluidSimGridRes;
	float3 pos = float3(DTid);

	//move 'backwards' equal to the velocity map at this point
	//to get prev pos of whats coming to this point
	//float3 pos = coords - deltaTime * VelocityMap.SampleLevel(BilinearSampler, coords, 0.0f);
	pos = pos - deltaTime * VelocityMap[DTid].xyz;

	//pos = (pos + 0.5f) * invFluidSimGridRes;
	float3 posUVW = PixelIndexToUVW(pos, gridRes);

	//write to output buffer which will be next frames data
	UavOutputMap[DTid] = InputMap.SampleLevel(LinearClampSampler, pos, 0.0f);
}