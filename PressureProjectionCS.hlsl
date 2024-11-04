#include "FluidSimHelpers.hlsli"

cbuffer ExternalData : register(b0) {
	float deltaTime;
	float invFluidSimGridRes; //(1/windowWidth, 1/windowHeigh)
	int gridRes;
};

RWTexture3D<float4> UavOutputMap : register (u0);
Texture3D<float4> VelocityMap : register (t0);
Texture3D<float4> PressureMap : register(t1);

//SamplerState PointSampler : register(s0);

[numthreads(GROUP_SIZE, GROUP_SIZE, GROUP_SIZE)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	//float3 coords = (float3(DTid)+0.5f) * invFluidSimGridRes;

	int3 coords = DTid;

	// Compute the gradient of pressure at the current cell by    
// taking central differences of neighboring pressure values.    
	float pLeft = PressureMap[GetLeftIndex(coords)].r;
	float pRight = PressureMap[GetRightIndex(coords, gridRes)].r;
	float pBottom = PressureMap[GetBottomIndex(coords)].r;
	float pTop = PressureMap[GetTopIndex(coords, gridRes)].r;
	float pBack = PressureMap[GetBackIndex(coords)].r;
	float pFront = PressureMap[GetFrontIndex(coords, gridRes)].r;

	float3 gradP = 0.5 * float3(pRight - pLeft, pTop - pBottom, pFront - pBack);
	// Project the velocity onto its divergence-free component by    
	// subtracting the gradient of pressure.    
	float3 vOld = VelocityMap[DTid];// VelocityMap.SampleLevel(PointSampler, coords, 0.0f);
	float3 vNew = vOld - gradP;
	UavOutputMap[DTid] = float4(vNew, 1);
}
