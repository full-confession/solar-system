

texture2D<float4> buffer : register(t0);
texture2D<float4> bloom : register(t1);
texture2D<uint> dither : register(t2);

SamplerState pointSamplerWrap : register(s1);
SamplerState linearSampler : register(s2);
SamplerState pointSampler : register(s3);



cbuffer PixelBuffer : register(b0)
{
	float width;
	float height;
	float bloomWidth;
	float bloomHeight;
}

float4 main(float4 position : SV_POSITION): SV_TARGET
{
	float2 uv = float2(position.x / width, position.y / height);

	//return pow(buffer.Sample(linearSampler, uv), 1.0f / 2.2f);

	float3 bloomColor = bloom.Sample(linearSampler, uv).rgb;

	float3 hdrColor = buffer.Sample(pointSampler, uv).rgb + bloomColor / 18.0f;

	float3 ldrColor = 1.0f - exp(-hdrColor * 2.0f);

	float3 noise = dither.Load(int3(position.x, position.y, 0) % 8).r / 64.0f;
	float3 ldrDitherLdr = ldrColor + noise / 255.0f;

	float3 ldrGammaDitherColor = pow(ldrDitherLdr, 1.0f / 2.2f);

	return float4(ldrGammaDitherColor, 1.0f);
}