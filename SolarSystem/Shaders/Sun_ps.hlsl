struct Input
{
	float4 screenPosition : SV_POSITION;
	float3 worldPosition : POSITION;
	float3 normal : NORMAL;
	float2 uv : UV;
};


texture2D<float4> emission : register(t0);
SamplerState linearSampler : register(s0);


static const float3 intensity = 100.0f;

float4 main(Input input): SV_TARGET
{
	float3 color = emission.Sample(linearSampler, input.uv).rgb;

	return float4(color * intensity, 1.0f);
}