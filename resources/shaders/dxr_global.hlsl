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
#ifndef __DXR_GLOBAL_HLSL__
#define __DXR_GLOBAL_HLSL__

#define MAX_RECURSION 3
//#define FOUR_X_A
//#define PATH_TRACING
//#define DEPTH_OF_FIELD
#define EPSILON 0.1
#define SOFT_SHADOWS
#define MAX_SHADOW_SAMPLES 1
#define RUSSIAN_ROULETTE
#define NO_PATH_TRACED_NORMALS
#define CALC_BITANGENT // calculate bitangent in the shader instead of using the bitangent uploaded

#ifdef FALLBACK
	#undef MAX_RECURSION
	#define MAX_RECURSION 1

	
	//#define NO_SHADOWS
#endif

RaytracingAccelerationStructure Scene : register(t0, space0);

#endif //__DXR_GLOBAL_HLSL__
