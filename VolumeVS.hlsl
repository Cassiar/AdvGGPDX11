cbuffer externalData:register(b0) {
	matrix world;
	matrix view;
	matrix projection;
};

struct VertexShaderInput {
	float3 position	: POSITION;
	float2 uv		: TEXCOORD;
	float3 normal	: NORMAL;
	float3 tangent	: TANGENT;
};

struct VertexToPixel {
	float4 screenPosition	: SV_POSITION;
	float3 uvw				: TEXCOORD;
	float3 worldPos			: POSITION;
};

VertexToPixel main( VertexShaderInput input)
{
	VertexToPixel output;

	//matrix math so calculate 'backwards'
	matrix worldViewProj = mul(projection, mul(view, world));
	output.screenPosition = mul(worldViewProj, float4(input.position, 1.0f));

	output.worldPos = mul(world, float4(input.position, 1.0f).xyz);

	//need to flip y to match tex coords
	output.uvw = saturate(sign(input.position));
	output.uvw.y = 1.0f - output.uvw.y;

	return output;
}