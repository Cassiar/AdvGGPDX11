#include "FluidSimHelpers.hlsli"
#define NUM_SAMPLES 256

//needs to match fluid field render types
#define RENDER_MODE_DEBUG -1
#define RENDER_MODE_BLEND 0
#define RENDER_MODE_ADD 1

cbuffer externalData : register(b0) {
	matrix invWorld;
	float3 cameraPosition;
	int renderMode;
	int raymarchSamples;
}

struct VertexToPixel {
	float4 screenPosition	: SV_POSITION;
	float3 uvw				: TEXCOORD;
	float3 worldPos : POSITION;
};

Texture3D VolumeTexture : register(t0);
SamplerState SamplerLinearClamp : register(s0);

bool RayAABBIntersection(float3 pos, float3 dir, float3 boxMin, float3 boxMax, out float t0, out float t1) {
	//invert direction and get test min and max
	float3 invDir = 1.0f / dir;
	float3 testMin = (boxMin - pos) * invDir;
	float3 testMax = (boxMax - pos) * invDir;

	float3 tmin = min(testMin, testMax);
	float3 tmax = max(testMin, testMax);

	//get max of tmin's xyz
	float2 t = max(tmin.xx, tmin.yz);
	t0 = min(t.x, t.y);

	t = min(tmax.xx, tmax.yz);
	t1 = min(t.x, t.y);

	return t0 <= t1;
}

float4 main(VertexToPixel input) : SV_TARGET
{
	//assume cube is 1x1x1 iwth offsets at 0.5 and -0.5
	const float3 aabbMin = float3(-0.5f, -0.5f, -0.5f);
	const float3 aabbMax = float3(0.5f, 0.5f, 0.5f);

	//get view direction in world space then local
	float3 pos = cameraPosition;
	float3 dir = normalize(input.worldPos - pos);

	float3 posLocal = mul(invWorld, float4(pos, 1)).xyz;
	float3 dirLocal = normalize(mul(invWorld, float4(dir, 1)).xyz);

	float nearHit;
	float farHit;
	RayAABBIntersection(posLocal, dirLocal, aabbMin, aabbMax, nearHit, farHit);

	if (nearHit < 0.0f) {
		nearHit = 0.0f;
	}

	//beginning and end ray positions
	float3 rayStart = posLocal + dirLocal * nearHit;
	float3 rayEnd = posLocal + dirLocal * farHit;

	float maxDist = farHit - nearHit;
	float3 currentPos = rayStart;
	float step = 1.73205f / raymarchSamples; //longest diagonal in cube
	float3 stepDir = step * dir;

	float4 finalColor = float4(0, 0, 0, 0);
	float totalDist = 0.0f;

	[loop]
	for (int i = 0; i < raymarchSamples && totalDist < maxDist; i++) {
		float uvw = currentPos + float3(0.5f, 0.5f, 0.5f);
		float4 color = VolumeTexture.SampleLevel(SamplerLinearClamp, uvw, 0);

		if (renderMode == RENDER_MODE_DEBUG) {
			finalColor += color * step;
			finalColor.a = 1;
		}
		else if (renderMode == RENDER_MODE_ADD) {
			//float4 colorAndDensity = VolumeTexture.SampleLevel(SamplerLinearClamp, uvw, 0);

			finalColor.rgb += color.rgb * color.a;
			finalColor.a += color.a;
		}
		else if (renderMode == RENDER_MODE_BLEND) {
			finalColor.rgb += color.rgb * color.a * (1.0f - finalColor.a);
			finalColor.a += color.a * (1.0f - finalColor.a);
			if (finalColor.a > 0.99f) {
				break;
			}
		}

		//continuing stepping
		currentPos += stepDir;
		totalDist += step;
	}

	return finalColor;
}