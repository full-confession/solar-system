#include "Common.hlsli"


texture2D<half4> buffer;
SamplerState linearSampler : register(s2);

cbuffer PixelBuffer : register(b0)
{
	float2 texSize;
	float2 texelSize;
}

half4 main(float4 position : SV_POSITION): SV_TARGET
{
	float2 uv = float2(position.x / texSize.x, position.y / texSize.y);
	half4 color = DownsampleBox13Tap(buffer, linearSampler, uv, texelSize);
	return color;
}
