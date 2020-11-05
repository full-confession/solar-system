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

texture2D<float4> albedoTex : register(t0);
texture1D<half4> transmittanceTex : register(t1);
texture1D<half4> irradianceTex : register(t2);
texture2D<half4> nightTex : register(t3);

SamplerState linearSamplerWrap : register(s0);
SamplerState linearSampler : register(s2);

static const float3 sun = 10.0f;
static const float3 F0 = 0.04f;
static const float metallic = 0.0f;
static const float PI = 3.14159265359f;

float3 GetTransmittnace(float zenithCos)
{
	float u = 1.0f - zenithCos;
	return transmittanceTex.Sample(linearSampler, u).rgb;
}

float3 GetIrradiance(float sunZenithCos)
{
	float u = (1.0f - sunZenithCos) / 2.0f;
	return irradianceTex.Sample(linearSampler, u).rgb;
}

float3 GetColor(float3 albedo, float metallic, float roughness, float NdotV, float NdotL, float NdotH, float VdotH, float3 mainLight, float3 irrLight);

float4 main(Input input): SV_TARGET
{
	float4 albedo = albedoTex.Sample(linearSamplerWrap, input.uv).rgba;
	float3 night = nightTex.Sample(linearSamplerWrap, input.uv).rgb;

	float3 N = normalize(input.normal);
	float3 V = normalize(cameraPos - input.worldPosition);
	float3 L = normalize(-lightDir);
	float3 H = normalize(V + L);

	float HdotV = saturate(dot(H, V));
	float NdotH = saturate(dot(N, H));
	float NdotV = saturate(dot(N, V));
	float NdotL = saturate(dot(N, L));

	float3 Tl = GetTransmittnace(NdotL);
	float3 Tv = GetTransmittnace(NdotV);
	float3 irradiance = GetIrradiance(dot(N, L));

	float3 color = GetColor(
		albedo.xyz,
		metallic,
		albedo.w,
		NdotV,
		NdotL,
		NdotH,
		HdotV,
		sun * Tl,
		sun * irradiance
	);

	color = color * Tv;



	return float4(color, 1.0f);
}

float DistributionGGX(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}
float GeometrySmith(float NdotV, float NdotL, float roughness)
{
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float3 GetColor(
	float3 albedo,
	float metallic,
	float roughness,

	float NdotV,
	float NdotL,
	float NdotH,
	float VdotH,
	float3 mainLight,
	float3 irrLight
)
{

	float3 F0 = 0.04f;
	F0 = lerp(F0, albedo, metallic);

	float NDF = DistributionGGX(NdotH, roughness);
	float G = GeometrySmith(NdotV, NdotL, roughness);
	float3 F = fresnelSchlick(VdotH, F0);

	float3 kS = F;
	float kD = 1.0f - kS;
	kD *= 1.0f - metallic;

	float3 numerator = NDF * G * F;
	float denominator = 4.0 * NdotV * NdotL;
	float3 specular = numerator / max(denominator, 0.001);

	float3 radiance = mainLight + irrLight;
	float3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

	return Lo;
}
