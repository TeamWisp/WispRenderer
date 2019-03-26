static const float FNEAR = 0.1f;
static const float FFAR = 1000.f;

float WeighCoC(float coc, float radius)
{
	//return coc >= radius;
	return saturate((coc - radius + 2) / 2);
}

float WeighColor(float3 color)
{
	return 1 / (1 + max(max(color.r, color.g), color.b));
}

float GetLinearDepth(float depth)
{
	float z = (2 * FNEAR) / (FFAR + FNEAR - (depth * (FFAR - FNEAR)));
	return z;
}

// Calculates the gaussian blur weight for a given distance and sigmas
float CalcGaussianWeight(int sampleDist, float sigma)
{
	float g = 1.0f / sqrt(2.0f * 3.14159 * sigma * sigma);
	return (g * exp(-(sampleDist * sampleDist) / (2 * sigma * sigma)));
}

//// Performs a gaussian blur in one direction
//float4 Blur(in Texture2D input, in SamplerState s, float2 uv, float2 texScale, float sigma, bool nrmlize)
//{
//	float2 tex_dims;
//	input.GetDimensions(tex_dims.x, tex_dims.y);
//
//	float4 color = 0;
//	float weightSum = 0.0f;
//	for (int i = -7; i < 7; i++)
//	{
//		float weight = CalcGaussianWeight(i, sigma);
//		weightSum += weight;
//		uv += (i / tex_dims) * texScale;
//		float4 samp = input.SampleLevel(s, uv);
//		color += samp * weight;
//	}
//
//	if (nrmlize)
//		color /= weightSum;
//
//	return color;
//}