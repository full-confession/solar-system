
struct Input
{
	float4 screenPosition : SV_POSITION;
	float3 worldPosition : POSITION;
	float3 normal : NORMAL;
	float3 center : CENTER;
	float3 centerN : CENTERN;
	float2 uv : UV;
};

cbuffer PixelBuffer : register(b0)
{
	float3 lightDir;
	float3 cameraPos;
}

texture2D<float4> albedoTex : register(t0);
texture2D<float4> ringsTex : register(t1);

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

bool IntersectPlane(float3 n, float3 p0, float3 l0, float3 l, out float r)
{
	float denom = dot(n, l);
	if(abs(denom) > 1e-6)
	{
		float3 p0l0 = p0 - l0;
		float t = dot(p0l0, n) / denom;
		if(t >= 0)
		{
			float3 p = l0 + l * t;
			float3 v = p - p0;
			float d2 = dot(v, v);
			//r = 0.0f;
			//return true;
			r = (d2 - 36.0f) / (156.25f - 36.0f);
			return (d2 >= 36.0f) && (d2 <= 156.25f);
		}
	}
	r = 0.0f;
	return false;
}

float4 main(Input input): SV_TARGET
{



	float3 albedo = albedoTex.Sample(linearSampler, input.uv).rgb;

	float3 N = normalize(input.normal);
	float3 V = normalize(cameraPos - input.worldPosition);
	float3 L = normalize(-lightDir);
	float3 H = normalize(V + L);

	float HdotV = saturate(dot(H, V));
	float NdotH = saturate(dot(N, H));
	float NdotV = saturate(dot(N, V));
	float NdotL = saturate(dot(N, L));

	float r;
	float3 rad = radiance;
	bool rings = IntersectPlane(normalize(input.centerN), input.center, input.worldPosition, L, r);
	if(rings)
	{
		float a = (1.0f - ringsTex.Sample(linearSampler, float2(r, 0.5f)).w);
		rad *= a;
	}


	float3 F = fresnelSchlick(HdotV, F0);
	float NDF = DistributionGGX(NdotH, roughness);
	float G = GeometrySmith(NdotV, NdotL, roughness);

	float3 numerator = NDF * G * F;
	float denominator = 4.0f * NdotV * NdotV;
	float3 specular = numerator / max(denominator, 0.001f);


	float3 kS = F;
	float3 kD = 1.0f - kS;
	kD *= 1.0f - metallic;


	float3 Lo = (kD * albedo / PI + specular) * rad * NdotL;
	float3 color = Lo;

	return float4(color, 1.0f);
}