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
	float pL = PressureMap[coords - float3(1, 0, 0)].r;
	float pR = PressureMap[coords + float3(1, 0, 0)].r;
	float pB = PressureMap[coords - float3(0, 1, 0)].r;
	float pT = PressureMap[coords + float3(0, 1, 0)].r;
	float pD = PressureMap[coords - float3(0, 0, 1)].r;
	float pU = PressureMap[coords + float3(0, 0, 1)].r;

	float3 gradP = 0.5 * float3(pR - pL, pT - pB, pU - pD);
	// Project the velocity onto its divergence-free component by    
	// subtracting the gradient of pressure.    
	float3 vOld = VelocityMap.SampleLevel(PointSampler, coords, 0.0f);
	float3 vNew = vOld - gradP;
	UavOutputMap[DTid] = float4(vNew, 0);
}
