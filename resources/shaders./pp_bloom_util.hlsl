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
#ifndef __PP_BLOOM_UTIL_HLSL__
#define __PP_BLOOM_UTIL_HLSL__

static const float gaussian_weights[15] = { 0.023089f, 0.034587f,	0.048689f,	0.064408f,	0.080066f,	0.093531f,	0.102673f,	0.105915f,	0.102673f,	0.093531f,	0.080066f,	0.064408f,	0.048689f,	0.034587f,	0.023089f };

#endif //__PP_BLOOM_UTIL_HLSL__