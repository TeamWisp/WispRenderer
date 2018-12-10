struct HitInfo
{
	float4 colorAndDistance;
};

// Attributes output by the raytracing when hitting a surface,
// here the barycentric coordinates
struct Attributes
{
	float2 bary;
};

RWTexture2D<float4> gOutput : register(u0);

RaytracingAccelerationStructure SceneBVH : register(t0, space0);

[shader("raygeneration")]
void RaygenEntry()
{
  // Initialize the ray payload
  HitInfo payload;
  payload.colorAndDistance = float4(0, 0, 0, 0);

  // Get the location within the dispatched 2D grid of work items
  // (often maps to pixels, so this could represent a pixel coordinate).
  uint2 launchIndex = DispatchRaysIndex().xy;
  float2 dims = float2(DispatchRaysDimensions().xy);
  float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);

  // Define a ray, consisting of origin, direction, and the min-max distance values
  RayDesc ray;
  ray.Origin = float3(d.x, -d.y, 1);
  ray.Direction = float3(0, 0, -1);
  ray.TMin = 0;
  ray.TMax = 100000;

  // Trace the ray
  TraceRay(
	  SceneBVH,
	  RAY_FLAG_NONE,
	  0xFF, // InstanceInclusionMask
	  0, // RayContributionToHitGroupIndex
	  0, // MultiplierForGeometryContributionToHitGroupIndex
	  0, // miss shader index
	  ray,
	  payload);

  gOutput[launchIndex] = float4(payload.colorAndDistance.rgb, 1.f);
}

[shader("miss")]
void MissEntry(inout HitInfo payload)
{
	payload.colorAndDistance = float4(0.0f, 1.0f, 0.0f, -1.f);
}

[shader("closesthit")]
void ClosestHitEntry(inout HitInfo payload, in BuiltInTriangleIntersectionAttributes attr)
{
	payload.colorAndDistance = float4(1, 0, 0, RayTCurrent());
}
