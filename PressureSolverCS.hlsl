#include "FluidSimHelpers.hlsli"

// Defines the input to this compute shader
// used to handle globabl variables
cbuffer ExternalData : register(b0) {
	float deltaTime;
	float invFluidSimGridRes; //(1/windowWidth, 1/windowHeigh)
};

RWTexture3D<float4> UavOutputMap : register (u0);
Texture3D<float4> VelocityDivergenceMap : register (t0);
Texture3D<float4> PressureMap : register (t1);

SamplerState BilinearSampler : register(s0);
SamplerState PointSampler : register(s1);

groupshared float pressureLDS[GROUP_SIZE + 2][GROUP_SIZE + 2][GROUP_SIZE + 2];

[numthreads(GROUP_SIZE, GROUP_SIZE, GROUP_SIZE)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GRTid : SV_GroupThreadID)
{
	//load data to LDS
	if ((GRTid.x < (GROUP_SIZE + 2) / 2) &&
		(GRTid.y < (GROUP_SIZE + 2) / 2) &&
		(GRTid.z < (GROUP_SIZE + 2) / 2))
	{
		float2 coordsNormalized = float2(DTid)*invFluidSimGridRes;
		coordsNormalized += GRTid.xyz * invFluidSimGridRes;

		float4 pressureSample = PressureMap.Gather(PointSampler, coordsNormalized);

		float topLeftPressure = pressureSample.w;
		float bottomLeftPressure -= pressureSample.x;
		float topRightPressure = pressureSample.z;
		float bottomRightPressure = pressureSample.y;

		pressureLDS[GRTid.x * 2][GRTid.y * 2][GRTid.z * 2] = topLeftPressure;
		pressureLDS[GRTid.x * 2][GRTid.y * 2][GRTid.z * 2] = topLeftPressure;
		pressureLDS[GRTid.x * 2][GRTid.y * 2][GRTid.z * 2] = topLeftPressure;
		pressureLDS[GRTid.x * 2][GRTid.y * 2][GRTid.z * 2] = topLeftPressure;

		//would need to do the same for any obstacles
	}

	GroupMemoryBarrierWithGroupSync();

	int3 coords = DTid;
	int3 ldsID = GRTid + int3(1, 1, 1);

	float left = pressureLDS[ldsID.x - 1][ldsID.y][ldsID.z];
	float right = pressureLDS[ldsID.x + 1][ldsID.y][ldsID.z];
	float bottom = pressureLDS[ldsID.x][ldsID.y + 1][ldsID.z];
	float top = pressureLDS[ldsID.x][ldsID.y - 1][ldsID.z];
	float back = pressureLDS[ldsID.x][ldsID.y][ldsID.z + 1];
	float front = pressureLDS[ldsID.x][ldsID.y][ldsID.z - 1];
	float center = pressureLDS[ldsID.x][ldsID.y][ldsID.z];

	//repeat for obstacle
	//then if adjacent cells are obstacles
	//set that value to be center

	float velocityDivergence = VelocityDivergenceMap.Load(coords).x;

	UavOutputMap[Dtid] = (left + right + bottom + top + back + front - velocityDivergence) / 6.0f;
}