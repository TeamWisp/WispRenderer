
// Structs
struct Light
{
	float3 pos;			//Position in world space for spot & point
	float radius;			//Radius for point, height for spot

	float3 color;			//Color
	uint tid;			//Type id; light_type_x

	float3 dir;			//Direction for spot & directional
	float angel;			//Angle for spot; in radians
};

struct Vertex
{
	float3 pos;
	float2 uv;
	float3 normal;
	float3 tangent;
	float3 bitangent;
};

struct Material
{
	float4x4 model;
	float idx_offset;
	float vertex_offset;
	float albedo_id;
	float normal_id;
};

// Buffers
RWTexture2D<float4> gOutput : register(u0);
RaytracingAccelerationStructure Scene : register(t0, space0);
ByteAddressBuffer g_indices : register(t4);
StructuredBuffer<Light> lights : register(t5);
StructuredBuffer<Vertex> g_vertices : register(t6);
StructuredBuffer<Material> g_materials : register(t7);

Texture2D g_textures[20] : register(t8);
SamplerState s0 : register(s0);

//RWTexture2D<float4> gOutput : register(u0);

//RaytracingAccelerationStructure Scene : register(t0, space0);
Texture2D gbuffer_albedo : register(t1);
Texture2D gbuffer_normal : register(t2);
Texture2D gbuffer_depth : register(t3);

// Additional helpers

typedef BuiltInTriangleIntersectionAttributes MyAttributes;
struct HitInfo
{
	float4 color;
	float3 origin;
	unsigned int depth;
};

cbuffer CameraProperties : register(b0)
{
	float4x4 view;
	float4x4 inv_view;
	float4x4 inv_projection;
	float4x4 inv_projection_view;
	float3 camera_position;
	float padding;

	float dep_light_radius;
	float metal;
	float roughness;
	float intensity;
};

struct Ray
{
	float3 origin;
	float3 direction;
};

static const uint light_type_point = 0;
static const uint light_type_directional = 1;
static const uint light_type_spot = 2;



float3 unpack_position(float2 uv, float depth, float4x4 proj_inv, float4x4 view_inv)
{
	const float4 ndc = float4(uv * 2.0 - 1.0, depth, 1.0);
	float4 vpos = mul(proj_inv, ndc);
	//vpos /= vpos.w;
	//float4 wpos = mul(view_inv, float4(vpos.xyz, 1.0));
	//return wpos.xyz;
	return (vpos / vpos.w).xyz;
}

[shader("raygeneration")]
void RaygenEntry()
{
	// Initialize the ray payload
	//HitInfo payload;
	//payload.color = float4(1, 1, 1, 1);

	//Ray temp_ray = GenerateCameraRay(DispatchRaysIndex().xy, camera_position, inv_projection_view);

	//// Define a ray, consisting of origin, direction, and the min-max distance values
	//RayDesc ray;
	//ray.Origin = temp_ray.origin;
	//ray.Direction = temp_ray.direction;
	//ray.TMin = 0;
	//ray.TMax = 10000.0;

	// Trace the ray
	//TraceRay(
	//	Scene,
	//	RAY_FLAG_NONE,
	//	~0, // InstanceInclusionMask
	//	0, // RayContributionToHitGroupIndex
	//	1, // MultiplierForGeometryContributionToHitGroupIndex
	//	0, // miss shader index
	//	ray,
	//	payload);

	
	float2 screenCo = float2(DispatchRaysIndex().xy) / float2(DispatchRaysDimensions().xy);
	screenCo.y = 1.0 - screenCo.y;

	float2 uv = float2((screenCo * DispatchRaysDimensions().xy).xy);

	float depth = gbuffer_depth[int2(uv.xy)].x;

	float3 wpos = unpack_position(screenCo, depth, inv_projection, inv_view);
	

	gOutput[DispatchRaysIndex().xy] = float4(screenCo.xy, 0.0, 1.0); //payload.color;//
	//gOutput[DispatchRaysIndex().xy] = float4(wpos.xyz, 1.0); //payload.color;//
	//gOutput[DispatchRaysIndex().xy] = float4(depth, 0.0, 0.0, 1.0); //payload.color;//gbuffer_depth[int2(uv.xy)].x
	//gOutput[DispatchRaysIndex().xy] = float4(gbuffer_normal[int2(uv.xy)].xyz, 1.0); //payload.color;//
}

[shader("miss")]
void MissEntry(inout HitInfo payload)
{
	payload.color = float4(170.0f / 255.f, 203.0f / 255.f, 1.0f, 1.f);
}

[shader("closesthit")]
void ClosestHitEntry(inout HitInfo payload, in MyAttributes attr)
{
    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    payload.color = float4(barycentrics, 1);
}
