#include "FluidSimHelpers.hlsli"

// Defines the input to this compute shader
// used to handle globabl variables
cbuffer ExternalData : register(b0) {
	float deltaTime;
	float invFluidSimGridRes; //(1/windowWidth, 1/windowHeigh)
};

RWTexture2D<float4> UavOutputMap : register (u0);
Texture2D<float4> InputMap : register (t0);
Texture2D<float4> VelocityMap : register (t1);
//Texture3D<float4> DensityMap : register (t2);

SamplerState BilinearSampler : register(s0);

[numthreads(GROUP_SIZE, GROUP_SIZE, GROUP_SIZE)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	//we're using center of each cell for sampling
	//get coord from thread id 
	float3 coords = (float3(DTid)+0.5f) * invFluidSimGridRes;

	//move 'backwards' equal to the velocity map at this point
	//to get prev pos of whats coming to this point
	float2 pos = coords.xy - deltaTime * VelocityMap.SampleLevel(BilinearSampler, coords.xy, 0.0f);

	//write to output buffer which will be next frames data
	UavOutputMap[DTid.xy] = InputMap.SampleLevel(BilinearSampler, pos, 0.0f);
}