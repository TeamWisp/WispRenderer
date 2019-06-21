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
#ifndef __FULLSCREEN_QUAD_HLSL__
#define __FULLSCREEN_QUAD_HLSL__

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

VS_OUTPUT main_vs(float2 pos : POSITION)
{
	VS_OUTPUT output;
	output.pos = float4(pos.x, pos.y, 0.0f, 1.0f);
	output.uv = 0.5 * (pos.xy + float2(1.0, 1.0));
    return output;
}

#endif //__FULLSCREEN_QUAD_HLSL__