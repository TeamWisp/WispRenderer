RWTexture2D<float4> gOutput : register(u0);
RaytracingAccelerationStructure Scene : register(t0, space0);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;
struct HitInfo
{
	float4 color;
};

[shader("raygeneration")]
void RaygenEntry()
{
    float2 lerpValues = (float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions();

  // Initialize the ray payload
  HitInfo payload;
  payload.color = float4(0, 0, 0, 0);

	float v_left = -1;
	float v_right = 1;
	float v_up = -1;
	float v_down = 1;

  float3 origin = float3(
	lerp(v_left, v_right, lerpValues.x),
	lerp(v_up, v_down, lerpValues.y),
	0.0f);

  // Define a ray, consisting of origin, direction, and the min-max distance values
  RayDesc ray;
  ray.Origin = origin;
  ray.Direction = float3(0, 0, 1);
  ray.TMin = 0.001;
  ray.TMax = 10000.0;

  // Trace the ray
  TraceRay(
	  Scene,
	  RAY_FLAG_NONE,
	  ~0, // InstanceInclusionMask
	  0, // RayContributionToHitGroupIndex
	  1, // MultiplierForGeometryContributionToHitGroupIndex
	  0, // miss shader index
	  ray,
	  payload);

  gOutput[DispatchRaysIndex().xy] = payload.color;
  //gOutput[DispatchRaysIndex().xy] = float4(lerpValues, 0, 1);
}

[shader("miss")]
void MissEntry(inout HitInfo payload)
{
	payload.color = float4(0.0f, 1.0f, 0.0f, 1.f);
}

[shader("closesthit")]
void ClosestHitEntry(inout HitInfo payload, in MyAttributes attr)
{
    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    payload.color = float4(barycentrics, 1);
}
