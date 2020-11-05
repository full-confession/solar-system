#include "Common.hlsli"


texture2D<half4> buffer;
SamplerState linearSampler : register(s2);

cbuffer PixelBuffer : register(b0)
{
	float2 texSize;
	float2 texelSize;
	float4 bloomThreshold; // x: threshold value (linear), y: threshold - knee, z: knee * 2, w: 0.25 / knee
}

half4 Prefilter(half4 color)
{
	color = QuadraticThreshold(color, bloomThreshold.x, bloomThreshold.yzw);
	return color;
}

half4 main(float4 position : SV_POSITION): SV_TARGET
{
	float2 uv = float2(position.x / texSize.x, position.y / texSize.y);
	half4 color = DownsampleBox13Tap(buffer, linearSampler, uv, texelSize);
	return Prefilter(color);
}
