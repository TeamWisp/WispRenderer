#ifndef __DXR_FUNCTIONS_HLSL__
#define __DXR_FUNCTIONS_HLSL__

float3 HitAttribute(float3 a, float3 b, float3 c, BuiltInTriangleIntersectionAttributes attr)
{
	float3 vertexAttribute[3];
	vertexAttribute[0] = a;
	vertexAttribute[1] = b;
	vertexAttribute[2] = c;

	return vertexAttribute[0] +
		attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
		attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

uint3 Load3x32BitIndices(ByteAddressBuffer buffer, uint offsetBytes)
{
	// Load first 2 indices
	return buffer.Load3(offsetBytes);
}

// Retrieve hit world position.
float3 HitWorldPosition()
{
	return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

float3 unpack_position(float2 uv, float depth, float4x4 inv_vp)
{
	// Get world space position
	const float4 ndc = float4(uv * 2.0 - 1.0, depth, 1.0);
	float4 wpos = mul(inv_vp, ndc);
	return (wpos.xyz / wpos.w).xyz;
}

float3 CalcPeturbedNormal(float3 normal, float3 normal_map, float3 tangent, float3 bitangent, float3 V, out float3 world_normal)
{
	float3x4 object_to_world = ObjectToWorld3x4();
	float3 N = normalize(mul(object_to_world, float4(normal, 0)));
	float3 T = normalize(mul(object_to_world, float4(tangent, 0)));
#ifndef CALC_BITANGENT
	const float3 B = normalize(mul(object_to_world, float4(bitangent, 0)));
#else
	T = normalize(T - dot(T, N) * N);
	float3 B = cross(N, T);
#endif
	const float3x3 TBN = float3x3(T, B, N);

	float3 fN = normalize(mul(normal_map, TBN));

	world_normal = N;

	return fN;
}

float3 CalcPeturbedNormal(float3 normal, float3 normal_map, float3 tangent, float3 bitangent, float3 V)
{
	float3 temp = 0;
	return CalcPeturbedNormal(normal, normal_map, tangent, bitangent, V, temp);
}


#endif //__DXR_FUNCTIONS_HLSL__
