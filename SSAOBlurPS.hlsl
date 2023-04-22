//author: cassiar beaver
//a pixel shader to blur the outcome of ssao
//so it doesn't have as many artifacts

cbuffer ExternalData : register(b0) {
	float2 pixelSize; //(1/windowWidth, 1/windowHeigh)
};

struct VertexToPixel
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

Texture2D SSAO : register(t0);

SamplerState ClampSampler : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
	float ao = 0; 
	for (float x = -1.5f; x <= 1.5f; x++) {
		for (float y = -1.5f; y <= 1.5f; y++) {
			ao += SSAO.Sample(ClampSampler, float2(x, y) * pixelSize + input.uv).r;
		}
	}

	//average resutls and return
	//we have a 4x4 blur, so 16 samples
	ao /= 16.0f;
	return float4(ao.rrr, 1);
}