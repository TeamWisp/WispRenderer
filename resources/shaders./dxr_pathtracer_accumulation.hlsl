/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __DXR_PATHTRACER_ACCUMULATION_HLSL__
#define __DXR_PATHTRACER_ACCUMULATION_HLSL__

#include "pp_util.hlsl"
#include "pp_hdr_util.hlsl"

Texture2D<float4> input : register(t0);
RWTexture2D<float4> output : register(u0);
SamplerState s0 : register(s0);

cbuffer Properties : register(b0)
{
	float frame_idx;
};


[numthreads(16, 16, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float4 current = input[DTid.xy]; // Last path tracer result
	float4 prev = output[DTid.xy]; // Previous path tracer output

	float accum_count = frame_idx; // 0 to x, the number of times the accumulation has ran.

	float4 color = (accum_count * prev + current) / (accum_count + 1); // accumulate

	output[DTid.xy] = color; // update the output with the accumulated result.
}

#endif //__DXR_PATHTRACER_ACCUMULATION_HLSL__