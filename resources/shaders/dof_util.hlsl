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

