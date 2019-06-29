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

//=================================================================================================
//
//  Baking Lab
//  by MJP and David Neubelt
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#ifndef __PP_DOF_COMPUTE_NEAR_MASK_HLSL__
#define __PP_DOF_COMPUTE_NEAR_MASK_HLSL__

#include "pp_dof_properties.hlsl"
#include "pp_dof_util.hlsl"

Texture2D input : register(t0);
RWTexture2D<float2> output : register(u0);
SamplerState s0 : register(s0);


//=================================================================================================
// Constants
//=================================================================================================
static const uint NumThreads = 16 * 16;



// -- shared memory
groupshared float Samples[NumThreads];

//=================================================================================================
// Computes a downscaled mask for the near field
//=================================================================================================
[numthreads(32, 32, 1)]
void main_cs(in uint3 GroupID : SV_GroupID, in uint3 GroupThreadID : SV_GroupThreadID,
	in uint ThreadIndex : SV_GroupIndex)
{
	uint2 textureSize;
	input.GetDimensions(textureSize.x, textureSize.y);

	uint2 samplePos = GroupID.xy * 16 + GroupThreadID.xy;
	samplePos = min(samplePos, textureSize - 1);

	float cocSample = input[samplePos].w;

	// -- store in shared memory
	Samples[ThreadIndex] = cocSample;
	GroupMemoryBarrierWithGroupSync();

	// -- reduce
	[unroll]
	for (uint s = NumThreads / 2; s > 0; s >>= 1)
	{
		if (ThreadIndex < s)
			Samples[ThreadIndex] = max(Samples[ThreadIndex], Samples[ThreadIndex + s]);

		GroupMemoryBarrierWithGroupSync();
	}

	if (ThreadIndex == 0)
		output[GroupID.xy] = Samples[0];
}

#endif //__PP_DOF_COC_HLSL__