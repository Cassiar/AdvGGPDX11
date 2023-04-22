//author: cassiar
//pixel shader to combine scene colors
//with ssao results

struct VertexToPixel
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

Texture2D SceneColorNoAmbient	: register(t0);
Texture2D Ambient				: register(t1);
Texture2D SSAOBlur				: register(t2);

SamplerState BasicSampler		: register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
	//sample all three textures
	float3 sceneColors = SceneColorNoAmbient.Sample(BasicSampler, input.uv).rgb;
	float3 ambient = Ambient.Sample(BasicSampler, input.uv).rgb;
	float ao = SSAOBlur.Sample(BasicSampler, input.uv).r;

	//combine all and return
	//gamma correction should have already happened
	return float4(ambient * ao + sceneColors, 1);
}