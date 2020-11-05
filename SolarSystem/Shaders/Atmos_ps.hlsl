struct Input
{
	float4 screenPosition : SV_POSITION;
	float3 worldPosition : POSITION;
	float3 normal : NORMAL;
	float2 uv : UV;
};

cbuffer PixelBuffer : register(b0)
{
	float3 lightDir;
	float3 cameraPos;
}

//texture2D<half4> scatteringTex : register(t0);
texture3D<half4> rayTex : register(t0);
texture3D<half4> mieTex : register(t1);

SamplerState linearSampler : register(s2);


//float3 GetScattering(float viewCos, float sunCos)
//{
//	float2 uv = float2(-viewCos, (1.0f - sunCos) / 2.0f);
//	return scatteringTex.Sample(linearSampler, uv).rgb;
//}

float ViewToU(float view)
{
	return 1.0f - view;
}


float SunToV(float sun)
{
	return (1.0 - sun) / 2.0f;
}

float ZenithToW(float zenith)
{
	return (1.0 - zenith) / 2.0f;
}


static const float pi = 3.141592653589793238462;

float RayleightPhase(float cos)
{
	return 3.0f / 16.0f / pi * (1.0f + cos * cos);
}

float MiePhase(float cos)
{
	float g = 0.8f;
	float g2 = g * g;
	return 3.0f / 8.0f / pi * (1.0f - g2) * (1.0 + cos * cos) / (2.0f + g2) / pow(1.0f + g2 - 2.0f * g * cos, 1.5f);
}

float4 main(Input input) : SV_TARGET
{ 
	float3 normal = normalize(input.normal);
	float3 view = normalize(cameraPos - input.worldPosition);
	float3 light = normalize(-lightDir);

	float viewCos = dot(view, normal);
	float sunCos = dot(light, normal);


	float3 viewProj = normalize(view - viewCos * normal);
	float3 lightProj = normalize(light - sunCos * normal);

	float azimuthCos = dot(viewProj, lightProj);

	float theta = dot(view, -light);

	float3 uv = float3(ViewToU(viewCos), SunToV(sunCos), ZenithToW(azimuthCos));

	float3 rayleight = rayTex.Sample(linearSampler, uv).rgb * RayleightPhase(theta);
	float3 mie = mieTex.Sample(linearSampler, uv).rgb * MiePhase(theta);

	return float4((mie + rayleight) * 15, 1.0f);

}