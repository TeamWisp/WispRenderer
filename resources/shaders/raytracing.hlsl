RWTexture2D<float4> gOutput : register(u0);
RaytracingAccelerationStructure Scene : register(t0, space0);
ByteAddressBuffer g_indices : register(t1);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;
struct HitInfo
{
	float4 color;
};

cbuffer CameraProperties : register(b0)
{
	float4x4 inv_projection_view;
	float3 camera_position;
};

struct Ray
{
	float3 origin;
	float3 direction;
};

uint3 Load3x32BitIndices(uint offsetBytes)
{
    uint3 retval;
 
	int index_size = 4; /* (32 bit) */

	// Load first 2 indices
 	retval.xy = g_indices.Load2(offsetBytes);
	// Offset the address by 2 indices than load z and [unknown]
 	retval.z = g_indices.Load2(offsetBytes + (4*2)).x;
 
    return retval;
}

inline Ray GenerateCameraRay(uint2 index, in float3 cameraPosition, in float4x4 projectionToWorld)
{
    float2 xy = index + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy / DispatchRaysDimensions().xy * 2.0 - 1.0;

	screenPos.y *= -1;
    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a world positon.
    float4 world = mul(float4(screenPos, 0, 1), projectionToWorld);
    world.xyz /= world.w;

    Ray ray;
    ray.origin = cameraPosition;
    ray.direction = normalize(world.xyz - ray.origin);

    return ray;
}

[shader("raygeneration")]
void RaygenEntry()
{
	// Initialize the ray payload
	HitInfo payload;
	payload.color = float4(1, 1, 1, 1);

	Ray temp_ray = GenerateCameraRay(DispatchRaysIndex().xy, camera_position, inv_projection_view);

	// Define a ray, consisting of origin, direction, and the min-max distance values
	RayDesc ray;
	ray.Origin = temp_ray.origin;
	ray.Direction = temp_ray.direction;
	ray.TMin = 0;
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
}

[shader("miss")]
void MissEntry(inout HitInfo payload)
{
	payload.color = float4(170.0f / 255.f, 203.0f / 255.f, 1.0f, 1.f);
}

[shader("closesthit")]
void ClosestHitEntry(inout HitInfo payload, in MyAttributes attr)
{
    uint index_size = 2;
    uint indices_per_triangle = 3;
    uint triangle_idx_stride = indices_per_triangle * index_size;
    uint base_idx = PrimitiveIndex() * triangle_idx_stride;

	const uint3 indices = Load3x32BitIndices(base_idx);

    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
	float3 indices_c = float3((float)indices.x / 255.f, (float)indices.y / 255.f, (float)indices.z) / 255.f;
    payload.color = float4(indices_c, 1);
}
