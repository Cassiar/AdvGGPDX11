cbuffer ExternalData : register(b0) {
	float deltaTime;
	float invFluidSimGridRes; //(1/windowWidth, 1/windowHeigh)
};

RWTexture3D<float4> UavOutputMap : register (u0);
Texture3D<float4> VelocityMap : register (u1);

SamplerState BilinearSampler : register(s0);
SamplerState PointSampler : register(s1); 

[numthreads(GROUP_SIZE, GROUP_SIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float3 coords = (float3(DTid)+0.5f) * invFluidSimGridRes;

	float3 velLeft = VelocityMap.Sample(PointSampler, coords - float3(1, 0, 0));
	float3 velRight = VelocityMap.Sample(PointSampler, coords + float3(1, 0, 0));
	float3 velBottom = VelocityMap.Sample(PointSampler, coords - float3(0, 1, 0));
	float3 velTop = VelocityMap.Sample(PointSampler, coords + float3(0, 1, 0));
	float3 velBack = VelocityMap.Sample(PointSampler, coords - float3(0, 0, 1));
	float3 velFront = VelocityMap.Sample(PointSampler, coords + float3(0, 0, 1));

	float velocityDivergence = 0.5f * (
		(velRight - velLeft) +
		(velTop - velBottom) +
		(velFront - velBack));

	UavOutputMap[DTid] = velocityDivergence.xxxx;
}