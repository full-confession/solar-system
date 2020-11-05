
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
SamplerState linearSampler : register(s0);

static const float3 radiance = 10.0f;
static const float3 F0 = 0.04f;
static const float roughness = 0.95f;
static const float metallic = 0.0f;
static const float PI = 3.14159265359f;

float3 fresnelSchlick(float HdotV, float3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
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

float4 main(Input input): SV_TARGET
{
	float3 albedo = albedoTex.Sample(linearSampler, input.uv).rgb;

	float3 N = normalize(input.normal);
	float3 V = normalize(cameraPos - input.worldPosition);
	float3 L = -lightDir;
	float3 H = normalize(V + L);

	float HdotV = saturate(dot(H, V));
	float NdotH = saturate(dot(N, H));
	float NdotV = saturate(dot(N, V));
	float NdotL = saturate(dot(N, L));

	float3 F = fresnelSchlick(HdotV, F0);
	float NDF = DistributionGGX(NdotH, roughness);
	float G = GeometrySmith(NdotV, NdotL, roughness);

	float3 numerator = NDF * G * F;
	float denominator = 4.0f * NdotV * NdotV;
	float3 specular = numerator / max(denominator, 0.001f);


	float3 kS = F;
	float3 kD = 1.0f - kS;
	kD *= 1.0f - metallic;


	float3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
	//float3 La = albedo * 0.0001f;
	//float Ln = saturate(dot(lightDir, N));
	//float3 albedo = albedoTex.Sample(linearSampler, input.uv).rgb;
	//float3 color = albedo * Ln;
	float3 color = Lo;

	return float4(color, 1.0f);
}