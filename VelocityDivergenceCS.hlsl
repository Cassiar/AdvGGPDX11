#include "FluidSimHelpers.hlsli"

cbuffer ExternalData : register(b0) {
	float deltaTime;
	float invFluidSimGridRes; //(1/windowWidth, 1/windowHeigh)
};

RWTexture3D<float4> UavOutputMap : register (u0);
Texture3D<float4> VelocityMap : register (t0);

SamplerState BilinearSampler : register(s0);
SamplerState PointSampler : register(s1); 

[numthreads(GROUP_SIZE, GROUP_SIZE, GROUP_SIZE)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float3 coords = (float3(DTid)+0.5f) * invFluidSimGridRes;

	float3 velLeft = VelocityMap[coords - float3(1, 0, 0)];
	float3 velRight = VelocityMap[coords + float3(1, 0, 0)];
	float3 velBottom = VelocityMap[coords - float3(0, 1, 0)];
	float3 velTop = VelocityMap[coords + float3(0, 1, 0)];
	float3 velBack = VelocityMap[coords - float3(0, 0, 1)];
	float3 velFront = VelocityMap[coords + float3(0, 0, 1)];

	float velocityDivergence = 0.5f * (
		(velRight.x - velLeft.x) +
		(velTop.y - velBottom.y) +
		(velFront.z - velBack.z));

	UavOutputMap[DTid] = velocityDivergence.xxxx;
}