// Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

constexpr float operator"" _deg(long double deg)
{
	return DirectX::XMConvertToRadians(static_cast<float>(deg));
}

constexpr float operator"" _rad(long double rad)
{
	return DirectX::XMConvertToDegrees(static_cast<float>(rad));
}

constexpr float operator"" _deg(unsigned long long int deg)
{
	return DirectX::XMConvertToRadians(static_cast<float>(deg));
}

constexpr float operator"" _rad(unsigned long long int rad)
{
	return DirectX::XMConvertToDegrees(static_cast<float>(rad));
}

constexpr std::size_t operator"" _kb(std::size_t kilobytes)
{
	return kilobytes * 1024;
}

constexpr std::size_t operator"" _mb(std::size_t megabytes)
{
	return megabytes * 1024 * 1024;
}