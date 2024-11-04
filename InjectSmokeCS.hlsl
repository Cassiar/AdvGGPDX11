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

	float3 posInGrid = float3(DTid);//in [0-gridSize] range
	float3 powUVW = PixelIndexToUVW(posInGrid, gridSize);//in[0-1] range

	//inject less if pos farther from center
	float dist = length(posUVW - injectPosition);
	//if there's no radius, use 0, otherwise we scale based on distance
	float injFalloff = injectRadius == 0.0 ? 0.0f : max(0, injectRadius - dist) / injectRadius;

	float4 oldColorAndDensity = DensityMap[DTid];

	//calc new values - color is a replacement, density is an add
	//if we're within the falloff area, use the new color
	float3 newColor = injFalloff > 0 ? injectColor : oldColorAndDensity.rgb;
	float newDensity = saturate(oldColorAndDensity.a + injectDensity * injFalloff);

	//update the outputs
	DensityOut[DTid] = float4(newColor, newDensity);
	TemperatureOut[DTid] = TemperatureMap[DTid].r + injectTemperature * injFalloff;
	VelocityOut[DTid] = VelocityMap[DTid] + float4(injFalloff > 0 ? injectVelocity : 0, 0);
}