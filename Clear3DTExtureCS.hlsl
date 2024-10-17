#include "FluidSimHelpers.hlsli"

cbuffer externalData : register(b0) {
	float4 clearColor;
	int channelCount;
}

RWTexture3D<float> ClearOut1 : register(u0);
RWTexture3D<float> ClearOut2 : register(u1);
RWTexture3D<float> ClearOut3 : register(u2);
RWTexture3D<float> ClearOut4 : register(u3);

[numthreads(GROUP_SIZE, GROUP_SIZE, GROUP_SIZE)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	switch (channelCount) {
	case 1: ClearOut1[DTid] = clearColor.r; break;
	case 2: ClearOut1[DTid] = clearColor.rg; break;
	case 3: ClearOut1[DTid] = clearColor.rgb; break;
	case 4: ClearOut1[DTid] = clearColor; break;
	}
}