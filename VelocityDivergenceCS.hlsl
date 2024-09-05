#include "FluidSimHelpers.hlsli"

cbuffer ExternalData : register(b0) {
	float deltaTime;
	float invFluidSimGridRes; //(1/windowWidth, 1/windowHeigh)
	int gridRes;
};

RWTexture3D<float4> UavOutputMap : register (u0);
Texture3D<float4> VelocityMap : register (t0);

//SamplerState BilinearSampler : register(s0);
SamplerState PointSampler : register(s0); 

[numthreads(GROUP_SIZE, GROUP_SIZE, GROUP_SIZE)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float3 coords = (float3(DTid)+0.5f) * invFluidSimGridRes;

					//VelocityMap.SampleLevel(PointSampler, coords, 0.0f);
	float3 velLeft = VelocityMap[GetLeftIndex(DTid)];
	float3 velRight = VelocityMap[GetRightIndex(DTid, gridRes)];
	float3 velBottom = VelocityMap[GetBottomIndex(DTid)];
	float3 velTop = VelocityMap[GetTopIndex(DTid, gridRes)];
	float3 velBack = VelocityMap[GetBackIndex(DTid)];
	float3 velFront = VelocityMap[GetFrontIndex(DTid, gridRes)];

	//float3 velLeft = VelocityMap.SampleLevel(PointSampler, GetLeftIndex(coords), 0.0f).xyz;
	//float3 velRight = VelocityMap.SampleLevel(PointSampler, GetRightIndex(coords, gridRes), 0.0f).xyz;
	//float3 velBottom = VelocityMap.SampleLevel(PointSampler, GetBottomIndex(coords), 0.0f).xyz;
	//float3 velTop = VelocityMap.SampleLevel(PointSampler, GetTopIndex(coords, gridRes), 0.0f).xyz;
	//float3 velBack = VelocityMap.SampleLevel(PointSampler, GetBackIndex(coords), 0.0f).xyz;
	//float3 velFront = VelocityMap.SampleLevel(PointSampler, GetFrontIndex(coords, gridRes), 0.0f).xyz;

	float velocityDivergence = 0.5f * (
		(velRight.x - velLeft.x) +
		(velTop.y - velBottom.y) +
		(velFront.z - velBack.z));

	UavOutputMap[DTid] = float4(velocityDivergence.r, 0, 0, 0);
}