#include "FluidSimHelpers.hlsli"

cbuffer ExternalData : register(b0) {
	float deltaTime;
	float invFluidSimGridRes; //(1/windowWidth, 1/windowHeigh)
};

RWTexture3D<float4> UavOutputMap : register (u0);
Texture3D<float4> VelocityMap : register (t0);
Texture3D<float4> PressureMap : register(t1);

SamplerState PointSampler : register(s0);

[numthreads(GROUP_SIZE, GROUP_SIZE, GROUP_SIZE)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float3 coords = (float3(DTid)+0.5f) * invFluidSimGridRes;

	// Compute the gradient of pressure at the current cell by    
// taking central differences of neighboring pressure values.    
	float pL = PressureMap.Sample(PointSampler, coords - float3(1, 0, 0));
	float pR = PressureMap.Sample(PointSampler, coords + float3(1, 0, 0));
	float pB = PressureMap.Sample(PointSampler, coords - float3(0, 1, 0));
	float pT = PressureMap.Sample(PointSampler, coords + float3(0, 1, 0));
	float pD = PressureMap.Sample(PointSampler, coords - float3(0, 0, 1));
	float pU = PressureMap.Sample(PointSampler, coords + float3(0, 0, 1));

	float3 gradP = 0.5 * float3(pR - pL, pT - pB, pU - pD);
	// Project the velocity onto its divergence-free component by    
	// subtracting the gradient of pressure.    
	float3 vOld = VelocityMap.Sample(PointSampler, coords);
	float3 vNew = vOld - gradP;
	UavOutputMap[DTid] = float4(vNew, 0);
}
}