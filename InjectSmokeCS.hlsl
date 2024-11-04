#include "FluidSimHelpers.hlsli"
cbuffer ExternalData : register(b0) {
	float injectRadius;//in UV coords
	float3 injectPosition; //also in uv coords

	float3 injectColor;
	float injectDensity;
	
	float3 injectVelocity;
	float injectTemperature;

	int gridSize;
}

Texture3D DensityMap : register(t0);
Texture3D TemperatureMap : register(t1);
Texture3D VelocityMap : register(t2);
RWTexture3D<float4> DensityOut : register(u0);
RWTexture3D<float4> TemperatureOut : register(u1);
RWTexture3D<float4> VelocityOut : register(u2);

[numthreads(GROUP_SIZE, GROUP_SIZE, GROUP_SIZE)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	//here'd be where we'd check for obstacles

	// Pixel position in [0-gridSize] range and UV coords [0-1] range
	float3 posInGrid = float3(DTid);
	float3 posUVW = PixelIndexToUVW(posInGrid, gridSize);

	// How much to inject based on distance?
	float dist = length(posUVW - injectPosition);
	float injFalloff = injectRadius == 0.0f ? 0.0f : max(0, injectRadius - dist) / injectRadius;

	// Grab the old values
	float4 oldColorAndDensity = DensityMap[DTid];

	// Calculate new values - color is a replacement, density is an add
	float3 newColor = injFalloff > 0 ? injectColor : oldColorAndDensity.rgb;
	float newDensity = saturate(oldColorAndDensity.a + injectDensity * injFalloff);

	// Spit out the updates
	DensityOut[DTid] = float4(newColor, newDensity);
	TemperatureOut[DTid] = TemperatureMap[DTid].r + injectTemperature * injFalloff;
	VelocityOut[DTid] = VelocityMap[DTid] + float4(injFalloff > 0 ? injectVelocity : 0, 0);
}