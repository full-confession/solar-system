#include "Common.hlsli"


texture2D<half4> mainTex : register(t0);
texture2D<half4> bloomTex : register(t1);

SamplerState linearSampler : register(s2);

cbuffer PixelBuffer : register(b0)
{
	float2 texSize;
	float2 texelSize;
	float4 bloomThreshold;
	float sampleScale;
}


half4 Combine(half4 bloom, float2 uv)
{
	half4 color = bloomTex.Sample(linearSampler, uv);
	return bloom + color;
}

half4 main(float4 position : SV_POSITION): SV_TARGET
{
	float2 uv = float2(position.x / texSize.x, position.y / texSize.y);
	half4 bloom = UpsampleTent(mainTex, linearSampler, uv, texelSize, sampleScale);
	return Combine(bloom, uv);
}
