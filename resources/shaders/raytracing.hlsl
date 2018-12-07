struct RayPayload
{
	float4 color;
};

[shader("raygeneration")]
void RaygenEntry()
{

}

[shader("miss")]
void MissEntry(inout RayPayload rayPayload)
{

}

[shader("closesthit")]
void ClosestHitEntry(inout RayPayload rayPayload, in BuiltInTriangleIntersectionAttributes attr)
{
}
