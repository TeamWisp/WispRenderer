Texture2D noisy_data : register(t0);
Texture2D depth : register(t1);
Texture2D kernel : register(t2);
RWTexture2D<float4> output   : register(u0);
SamplerState point_sampler   : register(s0);
SamplerState linear_sampler  : register(s1);

cbuffer CameraProperties : register(b0)
{
	float4x4 view;
	float4x4 projection;
	float4x4 inv_projection;
	float4x4 inv_view;
	uint is_hybrid;
};

cbuffer DenoiserSettings : register(b1)
{
    float2 direction;
    float depth_contrast;
};

static float SPACE = 6.0;
static float RANGE = 0.05;

static float offset[20] = { 0.0, 1.0, 2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,10.0,11.0,12.0,13.0,14.0,15.0,16.0,17.0,18.0,19.0 };

float CalcWeight(float2 offset)
{
    float x = length(offset);
    float sigma = SPACE;
    float mu = 0.0;
    float a = 1.0 / (sigma * sqrt(2.0*3.1415));
    float exponent = -0.5*(pow((x - mu) / sigma, 2.0));
    return a * exp(exponent);
}

float CalcWeightCenter(float center, float difference)
{
    float x = difference;
    float sigma = RANGE;
    float mu = center;
    float a = 1.0 / (sigma * sqrt(2.0*3.1415));
    float exponent = -0.5*(pow((x - mu) / sigma, 2.0));
    float ret = a * exp(exponent);
    return ret;
}

[numthreads(16,16,1)]
void shadow_denoiser_cs(int3 dispatch_thread_id : SV_DispatchThreadID)
{
    float2 screen_size = float2(0.f, 0.f);
	output.GetDimensions(screen_size.x, screen_size.y);

    float2 kernel_size = float2(0.f, 0.f);
    kernel.GetDimensions(kernel_size.x, kernel_size.y);

	float2 uv = float2(dispatch_thread_id.x / screen_size.x, dispatch_thread_id.y / screen_size.y);

	float2 screen_coord = int2(dispatch_thread_id.x, dispatch_thread_id.y);

    const float depth_f = depth[screen_coord].r;

    // We are only interested in the depth here
    float4 ndcCoords = float4(0, 0, depth_f, 1.0f);

    // Unproject the vector into (homogenous) view-space vector
    float4 viewCoords = mul(inv_projection, ndcCoords);

    // Divide by w, which results in actual view-space z value
    float linearDepth = viewCoords.z / viewCoords.w;

    float weights = 0.0;

    float accum = 0.0;

    float2 center = kernel_size/2.0;

    for(int i = 0; i < int(kernel_size.x); ++i)
    {
        float2 kernel_location = float2(i, i) * direction + center * float2(direction.y, direction.x);
        float2 coord = float2(kernel_location.x - center.x, kernel_location.y - center.y) + screen_coord;
        if(coord.x < 0 || coord.y < 0 || coord.x > screen_size.x || coord.y > screen_size.y)
        {

        }
        else
        {
            float shadow_data = noisy_data[int2(coord.x, coord.y)].r;
            float depth_data = depth[int2(coord.x, coord.y)].r;
            float4 ndcCoords = float4(0, 0, depth_data, 1.0f);
            float4 viewCoords = mul(inv_projection, ndcCoords);
            depth_data = viewCoords.z / viewCoords.w;
            float weight = lerp(kernel[int2(kernel_location.x, kernel_location.y)], 0.0, clamp(abs(linearDepth - depth_data)*depth_contrast, 0.0, 1.0));
            accum += shadow_data*weight;
            weights += weight;
        }            
        
    }

    accum /= weights;

    output[int2(dispatch_thread_id.xy)] = accum;
}