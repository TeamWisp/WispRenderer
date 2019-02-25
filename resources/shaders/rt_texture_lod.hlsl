float4x4 inverse(float4x4 m) {
    float n11 = m[0][0], n12 = m[1][0], n13 = m[2][0], n14 = m[3][0];
    float n21 = m[0][1], n22 = m[1][1], n23 = m[2][1], n24 = m[3][1];
    float n31 = m[0][2], n32 = m[1][2], n33 = m[2][2], n34 = m[3][2];
    float n41 = m[0][3], n42 = m[1][3], n43 = m[2][3], n44 = m[3][3];

    float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
    float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
    float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
    float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

    float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
    float idet = 1.0f / det;

    float4x4 ret;

    ret[0][0] = t11 * idet;
    ret[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet;
    ret[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet;
    ret[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet;

    ret[1][0] = t12 * idet;
    ret[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet;
    ret[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet;
    ret[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet;

    ret[2][0] = t13 * idet;
    ret[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet;
    ret[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet;
    ret[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet;

    ret[3][0] = t14 * idet;
    ret[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet;
    ret[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet;
    ret[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet;

    return ret;
}

struct RayCone
{
	float width;
	float spread_angle;
};

struct SurfaceHit
{
	float3 pos;
	float3 normal;
	float3 surface_spread_angle;
	float dist;
};

float2 WorldToScreen(float3 pos, float4x4 view, float4x4 proj)
{
	float width = 1280;
	float height = 720;

	float4x4 iv = inverse(view);
	float4x4 ip = inverse(proj);

	float4 p = mul(ip, mul(iv, float4(pos, 1)));

	float2 retval = p.xy / p.w;
	retval.x = (retval.x + 1) * (width / 2);
	retval.y = (retval.y + 1) * (height / 2);

	return retval;
}

//pa
float ComputeTriangleArea(float2 P0, float2 P1, float2 P2)
{
	return length((P1.x - P0.x) * (P2.y - P0.y) - (P2.x - P0.x) * (P1.y - P0.y));
	return sqrt(length((P1 - P0) * (P2 - P0)));
}

//ta
float ComputeTextureCoordsArea(float2 P0, float2 P1, float2 P2, float2 UV0, float2 UV1, float2 UV2, Texture2D T)
{
	float w, h;
	T.GetDimensions(w, h);
	return w * h * length((UV1.x - UV0.x) * (UV2.y - UV0.y) - (UV2.x - UV0.x) * (UV1.y - UV0.y));
}

float GetTriangleLODConstant(float2 P0, float2 P1, float2 P2, float2 UV0, float2 UV1, float2 UV2, Texture2D T)
{
	float P_a = ComputeTriangleArea(P0, P1, P2);
	float T_a = ComputeTextureCoordsArea(P0, P1, P2, UV0, UV1, UV2, T);
	return 0.5 * log2(T_a / P_a);
}

float ComputeTextureLOD(RayCone cone, float3 V, float3 N, float2 P0, float2 P1, float2 P2, float2 UV0, float2 UV1, float2 UV2, Texture2D T)
{
	float w, h;
	T.GetDimensions(w, h);

	float lambda = GetTriangleLODConstant(P0, P1, P2, UV0, UV1, UV2, T);
	lambda += log2(abs(cone.width));
	lambda += 0.5 * log2(w * h);
	lambda -= log2(abs(dot(V, N)));
	return lambda;
}

float PixelSpreadAngle(float vertical_fov, float output_height)
{
	return atan((tan(vertical_fov / 2) * 2) / output_height);
}

float3 dumb_ddx(Texture2D t, float3 v)
{
	float2 pixel = DispatchRaysIndex().xy;
	float3 left = t[float2(max(0, pixel.x-1), pixel.y)].xyz;
	float3 right = t[float2(min(1280, pixel.x+1), pixel.y)].xyz;

	return right - left;
}

float3 dumb_ddy(Texture2D t, float3 v)
{
	float2 pixel = DispatchRaysIndex().xy;
	float3 top = t[float2(pixel.x, max(0, pixel.y-1))].xyz;
	float3 bottom = t[float2(pixel.x, min(720, pixel.y+1))].xyz;

	return top - bottom;
}

float3 dumb_ddx_depth(Texture2D t, float4x4 inv_vp, float3 v)
{
	float2 uv = float2(DispatchRaysIndex().xy) / float2(DispatchRaysDimensions().xy - 1);

	float2 pixel = DispatchRaysIndex().xy;
	float left = t[float2(max(0, pixel.x-1), pixel.y)].x;
	float right = t[float2(min(1280, pixel.x+1), pixel.y)].x;

	// Get world space position
	const float4 ndc = float4(uv * 2.0 - 1.0, right - left, 1.0);
	float4 wpos = mul(inv_vp, ndc);
	return (wpos.xyz / wpos.w).xyz;
}

float3 dumb_ddy_depth(Texture2D t, float4x4 inv_vp, float3 v)
{
	float2 uv = float2(DispatchRaysIndex().xy) / float2(DispatchRaysDimensions().xy - 1);

	float2 pixel = DispatchRaysIndex().xy;
	float top = t[float2(pixel.x, max(0, pixel.y-1))].x;
	float bottom = t[float2(pixel.x, min(720, pixel.y+1))].x;

	// Get world space position
	const float4 ndc = float4(uv * 2.0 - 1.0, top - bottom, 1.0);
	float4 wpos = mul(inv_vp, ndc);
	return (wpos.xyz / wpos.w).xyz;
}

float ComputeSurfaceSpreadAngle(Texture2D g_P, Texture2D g_N, float4x4 inv_vp, float3 P, float3 N)
{
	float3 aPx = dumb_ddx_depth(g_P, inv_vp, P);
	float3 aPy = dumb_ddy_depth(g_P, inv_vp, P);

	float3 aNx = dumb_ddx(g_N, N);
	float3 aNy = dumb_ddx(g_N, N);

	float k1 = 1;
	float k2 = 1;
	
	float s = sign(dot(aPx, aNx) + dot(aPy, aNy));

	return 2 * k1 * s * sqrt(dot(aNx, aNx) + dot(aNy, aNy)) + k2;
}

RayCone Propagate(RayCone cone, float surface_spread_angle, float hit_dist)
{
	RayCone new_cone;
 	new_cone.width = cone.spread_angle * hit_dist + cone.width;
	new_cone.spread_angle = cone.spread_angle + surface_spread_angle;

	return new_cone;
}

RayCone ComputeRayConeFromGBuffer(SurfaceHit hit)
{
	float vfov = 1.5708; // 90 degrees
	RayCone cone;
	cone.width = 0;
	cone.spread_angle = PixelSpreadAngle(vfov, 720);

	return Propagate(cone, hit.surface_spread_angle, hit.dist);
}
