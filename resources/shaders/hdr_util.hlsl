float3 linearToneMapping(float3 color, float exposure, float gamma)
{
	color = clamp(exposure * color, 0.f, 1.f);
	color = pow(color, 1.f / gamma);
	return color;
}

float3 simpleReinhardToneMapping(float3 color, float exposure, float gamma)
{
	color *= exposure / (1. + color / exposure);
	color = pow(color, 1. / gamma);
	return color;
}

float3 lumaBasedReinhardToneMapping(float3 color, float gamma)
{
	float luma = dot(color, float3(0.2126, 0.7152, 0.0722));
	float toneMappedLuma = luma / (1. + luma);
	color *= toneMappedLuma / luma;
	color = pow(color, (1. / gamma));
	return color;
}

float3 whitePreservingLumaBasedReinhardToneMapping(float3 color, float gamma)
{
	float white = 2.;
	float luma = dot(color, float3(0.2126, 0.7152, 0.0722));
	float toneMappedLuma = luma * (1. + luma / (white*white)) / (1. + luma);
	color *= toneMappedLuma / luma;
	color = pow(color, (1. / gamma));
	return color;
}

float3 RomBinDaHouseToneMapping(float3 color, float gamma)
{
	color = exp(-1.0 / (2.72*color + 0.15));
	color = pow(color, (1. / gamma));
	return color;
}

float3 filmicToneMapping(float3 color)
{
	color = max(float3(0., 0., 0.), color - float3(0.004, 0.004, 0.004));
	color = (color * (6.2 * color + .5)) / (color * (6.2 * color + 1.7) + 0.06);
	return color;
}

float3 GrayscaleToneMapping(float3 color) {
	float gray = 0.299 * color.r + 0.587 * color.g + 0.114 * color.b;
	color.r = gray;
	color.g = gray;
	color.b = gray;
	return color;
}

float3 Uncharted2ToneMapping(float3 color, float gamma, float exposure)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	color *= exposure;
	color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	color /= white;
	color = pow(color, (1. / gamma));
	return color;
}

float4x4 brightnessMatrix(float brightness)
{
	return float4x4(1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		brightness, brightness, brightness, 1);
}

float3 ApplyHue(float3 color, float hue_in) {
	float3 result = color;

	const float4  kRGBToYPrime = float4(0.299, 0.587, 0.114, 0.0);
	const float4  kRGBToI = float4(0.596, -0.275, -0.321, 0.0);
	const float4  kRGBToQ = float4(0.212, -0.523, 0.311, 0.0);

	const float4  kYIQToR = float4(1.0, 0.956, 0.621, 0.0);
	const float4  kYIQToG = float4(1.0, -0.272, -0.647, 0.0);
	const float4  kYIQToB = float4(1.0, -1.107, 1.704, 0.0);

	float YPrime = dot(color, kRGBToYPrime);
	float I = dot(color, kRGBToI);
	float Q = dot(color, kRGBToQ);

	float hue = atan2(Q, I);
	float chroma = sqrt(I * I + Q * Q);

	hue += hue_in;

	Q = chroma * sin(hue);
	I = chroma * cos(hue);

	float4 yIQ = float4(YPrime, I, Q, 0.0);
	result.r = dot(yIQ, kYIQToR);
	result.g = dot(yIQ, kYIQToG);
	result.b = dot(yIQ, kYIQToB);

	return result;
}

float4x4 ContrastMatrix(float contrast)
{
	float t = (1.0 - contrast) / 2.0;

	return float4x4(contrast, 0, 0, 0,
		0, contrast, 0, 0,
		0, 0, contrast, 0,
		t, t, t, 1);

}

float3 AllTonemappingAlgorithms(float3 color, float rotation, float exposure, float gamma) {
	float3 result = color;

	float n = 9;
	int i = int(n * (rotation / 2));

	if (i == 0) result = linearToneMapping(color, exposure, gamma);
	if (i == 1) result = simpleReinhardToneMapping(color, exposure, gamma);
	if (i == 2) result = lumaBasedReinhardToneMapping(color, gamma);
	if (i == 3) result = whitePreservingLumaBasedReinhardToneMapping(color, gamma);
	if (i == 4) result = RomBinDaHouseToneMapping(color, gamma);
	if (i == 5) result = filmicToneMapping(color);
	if (i == 6) result = Uncharted2ToneMapping(color, exposure, gamma);
	if (i == 7) result = GrayscaleToneMapping(color);
	if (i == 8) result = color;

	return result;
}
