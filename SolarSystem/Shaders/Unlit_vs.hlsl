struct Input
{
	float3 position : POSITION;
	float4 color : COLOR;
};

struct Output
{
	float4 screenPosition : SV_POSITION;
	float3 worldPosition : POSITION;
	float4 color : COLOR;
};


cbuffer PerObject : register(b0)
{
	float4x4 WVP;
	float4x4 WORLD;
}

Output main(Input i)
{
	Output o;
	o.screenPosition = mul(WVP, float4(i.position, 1.0f));
	o.worldPosition = mul(WORLD, float4(i.position, 1.0f)).xyz;
	o.color = i.color;
	return o;
}