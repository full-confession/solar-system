
struct Input
{
	float4 screenPosition : SV_POSITION;
	float3 worldPosition : POSITION;
	float3 center : CENTER;
	float2 uv : UV;
};

texture2D<float4> albedoTex : register(t0);
SamplerState linearSampler : register(s0);

static const float3 radiance = 10.0f;
static const float PI = 3.14159265359f;

cbuffer PixelBuffer : register(b0)
{
	float3 lightDir;
	float3 cameraPos;
}

bool RaySphereEntryHit(float3 center, float radius, float3 origin, float3 direction, out float3 hitPoint)
{
	float3 oc = origin - center;
	float a = dot(direction, direction);
	float b = 2.0f * dot(oc, direction);
	float c = dot(oc, oc) - radius * radius;

	float D = b * b - 4 * a * c;
	if(D >= 0.0f)
	{
		float t = (-b - sqrt(D)) / (2.0f * a);
		if(t >= 0.0f)
		{
			hitPoint = origin + direction * t;
			return true;
		}
	}
	return false;
}



float4 main(Input input): SV_TARGET
{

	//float3 position = WORLD

	float3 l = normalize(-lightDir);
	float3 hit = 0.0f;
	bool planetHit = RaySphereEntryHit(
		input.center,
		5.0f,
		input.worldPosition,
		l,
		hit
	);

	float3 r = radiance;
	if(planetHit)
	{
		hit = normalize(hit - input.center);
		r = r * (1.0f - saturate(dot(hit, -l) / 0.2));
	}


	float4 albedo = albedoTex.Sample(linearSampler, input.uv).rgba;
	float3 color = albedo.rgb * r / (2.0f * PI);
	return float4(color, albedo.a);
}