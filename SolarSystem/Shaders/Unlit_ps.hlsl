
struct Input
{
	float4 screenPosition : SV_POSITION;
	float3 worldPosition : POSITION;
	float4 color : COLOR;
};



float4 main(Input i): SV_TARGET
{
	return i.color;
}