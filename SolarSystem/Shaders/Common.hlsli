#define EPSILON 1.0e-4

float Max3(float a, float b, float c)
{
	return max(max(a, b), c);
}

//
// Quadratic color thresholding
// curve = (threshold - knee, knee * 2, 0.25 / knee)
//
half4 QuadraticThreshold(half4 color, half threshold, half3 curve)
{
	// Pixel brightness
	half br = Max3(color.r, color.g, color.b);

	// Under-threshold part: quadratic curve
	half rq = clamp(br - curve.x, 0.0, curve.y);
	rq = curve.z * rq * rq;

	// Combine and apply the brightness response curve.
	color *= max(rq, br - threshold) / max(br, EPSILON);

	return color;
}

// Better, temporally stable box filtering
// [Jimenez14] http://goo.gl/eomGso
// . . . . . . .
// . A . B . C .
// . . D . E . .
// . F . G . H .
// . . I . J . .
// . K . L . M .
// . . . . . . .
half4 DownsampleBox13Tap(texture2D<half4> tex, SamplerState texSampler, float2 uv, float2 texelSize)
{
	half4 A = tex.Sample(texSampler, uv + texelSize * float2(-1.0f, -1.0f));
	half4 B = tex.Sample(texSampler, uv + texelSize * float2(0.0f, -1.0f));
	half4 C = tex.Sample(texSampler, uv + texelSize * float2(1.0f, -1.0f));
	half4 D = tex.Sample(texSampler, uv + texelSize * float2(-0.5f, -0.5f));
	half4 E = tex.Sample(texSampler, uv + texelSize * float2(0.0f, -0.5f));
	half4 F = tex.Sample(texSampler, uv + texelSize * float2(-1.0f, 0.0f));
	half4 G = tex.Sample(texSampler, uv + texelSize);
	half4 H = tex.Sample(texSampler, uv + texelSize * float2(1.0f, 0.0f));
	half4 I = tex.Sample(texSampler, uv + texelSize * float2(-0.5f, 0.5f));
	half4 J = tex.Sample(texSampler, uv + texelSize * float2(0.5f, 0.5f));
	half4 K = tex.Sample(texSampler, uv + texelSize * float2(-1.0f, 1.0f));
	half4 L = tex.Sample(texSampler, uv + texelSize * float2(0.0f, 1.0f));
	half4 M = tex.Sample(texSampler, uv + texelSize * float2(1.0f, 1.0f));

	half2 div = (1.0h / 4.0h) * half2(0.5h, 0.125h);

	half4 o = (D + E + I + J) * div.x;
	o += (A + B + G + F) * div.y;
	o += (B + C + H + G) * div.y;
	o += (F + G + L + K) * div.y;
	o += (G + H + M + L) * div.y;

	return o;
}

// 9-tap bilinear upsampler (tent filter)
half4 UpsampleTent(texture2D<half4> tex, SamplerState texSampler, float2 uv, float2 texelSize, float4 sampleScale)
{
	float4 d = texelSize.xyxy * float4(1.0f, 1.0f, -1.0f, 0.0f) * sampleScale;

	half4 s;
	s = tex.Sample(texSampler, uv - d.xy);
	s += tex.Sample(texSampler, uv - d.wy) * 2.0h;
	s += tex.Sample(texSampler, uv - d.zy);

	s += tex.Sample(texSampler, uv + d.zw) * 2.0h;
	s += tex.Sample(texSampler, uv);
	s += tex.Sample(texSampler, uv + d.xw) * 2.0h;

	s += tex.Sample(texSampler, uv + d.zy);
	s += tex.Sample(texSampler, uv + d.wy) * 2.0h;
	s += tex.Sample(texSampler, uv + d.xy);

	return s * (1.0 / 16.0);
}