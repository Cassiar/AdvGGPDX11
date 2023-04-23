//author: cassiar beaver
//a pixel shader to handle the first 
//step of ssao calcs

#define NUM_OFFSETS 64

cbuffer ExternalData : register(b0) {
	matrix viewMatrix; //camera view
	matrix projMatrix; //camera projection
	matrix invProjMatrix; //inverse of camera project

	float4 offsets[NUM_OFFSETS];
	float ssaoRadius;
	int ssaoSamples;//no more than above value
	float2 randomTextureScreenScale; //width/4 and height/4
}

struct VertexToPixel
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

//needs 3 textures
Texture2D Normals : register(t0);
Texture2D Depths : register(t1);
Texture2D Randoms : register(t2);

SamplerState BasicSampler : register(s0);
SamplerState ClampSampler : register(s1);//used for screen edges

//helper functions to convert data
float3 ViewSpaceFromDepth(float depth, float2 uv) {
	//convert to NDC
	uv.y = 1.0f - uv.y;
	uv = uv * 2.0f - 1.0f;

	float4 screenPos = float4(uv, depth, 1.0f);

	//convert to view space
	float4 viewPos = mul(invProjMatrix, screenPos);
	return viewPos.xyz / viewPos.w;
}

//get the uv position from view space position
float2 UVFromViewSpacePosition(float3 viewSpacePosition) {
	//apply the proj mat then perspective divide
	float4 samplePosScreen = mul(projMatrix, float4(viewSpacePosition, 1));
	samplePosScreen.xyz /= samplePosScreen.w;

	//adjust from NDCs to UV coords
	//don't forget to flip the y
	samplePosScreen.xy = samplePosScreen.xy * 0.5f + 0.5f;
	samplePosScreen.y = 1.0f - samplePosScreen.y;
	
	//just the uv
	return samplePosScreen.xy;
}

float4 main(VertexToPixel input) : SV_TARGET
{
	//sample depth first and exit early if sky box
	float pixelDepth = Depths.Sample(ClampSampler, input.uv).r;
	if (pixelDepth == 1.0f) {
		return float4(1, 1, 1, 1);
	}

	//get view space position
	float3 pixelPosViewSpace = ViewSpaceFromDepth(pixelDepth, input.uv);

	//sample the 4x4 random tex, assume it already holds normalized vec3s
	float3 randomDir = Randoms.Sample(BasicSampler, input.uv * randomTextureScreenScale).xyz;

	//sample normal and convert to view space
	float3 normal = Normals.Sample(BasicSampler, input.uv).xyz * 2.0f - 1.0f;
	normal = normalize(mul((float3x3)viewMatrix, normal));

	//calculate tangent, bitan, and normal mat
	float3 tangent = normalize(randomDir - normal * dot(randomDir, normal));
	float3 bitangent = cross(tangent, normal);
	float3x3 TBN = float3x3(tangent, bitangent, normal);

	//main loop to check near pixels for occluders
	float ao = 0.0f;
	for (int i = 0; i < ssaoSamples; i++) {
		//roate offset, scale and apply to position
		float3 samplePosView = pixelPosViewSpace + mul(offsets[i].xyz, TBN) * ssaoRadius;

		//get the UV coord of this position
		float2 samplePosScreen = UVFromViewSpacePosition(samplePosView);

		//sample the nearby depth and convet to view space
		float sampleDepth = Depths.SampleLevel(ClampSampler, samplePosScreen.xy, 0).r;
		float sampleZ = ViewSpaceFromDepth(sampleDepth, samplePosScreen.xy).z;

		//compare depths, and fade result based on range
		//far away objects aren't occluded
		float rangeCheck = smoothstep(0.0f, 1.0f, ssaoRadius / abs(pixelPosViewSpace.z - sampleZ));
		ao += (sampleZ < samplePosView.z ? rangeCheck : 0.0f);
	}

	//average the results, flip and return as greyscale
	ao = 1.0f - ao / ssaoSamples;
	return float4(ao.rrr, 1);
}