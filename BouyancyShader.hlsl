#include "FluidSimHelpers.hlsli"

cbuffer ExternalData : register(b0)
{
	float densityWeight;
	float temperatureBuoyancy;
	float ambientTemperature;
}

Texture3D VelocityMap : register(t0);
Texture3D DensityMap : register(t1);
Texture3D TemperatureMap : register(t2);
RWTexture3D<float4> VelocityOut : register(u0);

[numthreads(GROUP_SIZE, GROUP_SIZE, GROUP_SIZE)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	//check for obstacles and exit early if so
	//TODO

	//get temp
	float thisTemp = TemperatureMap[DTid].r;
	float density = DensityMap[DTid].a;

	// From: http://web.stanford.edu/class/cs237d/smoke.pdf
	float3 buoyancyForce = float3(0, 1, 0) *
		(-densityWeight * density + temperatureBuoyancy * (thisTemp - ambientTemperature));
	
	//add bouyancy force to cur velocity
	VelocityOut[DTid] = float4(VelocityMap[DTid].xyz + buoyancyForce, 1);
}