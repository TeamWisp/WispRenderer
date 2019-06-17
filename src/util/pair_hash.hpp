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

namespace util
{

	struct PairHash {
		template <class T1, class T2>
		std::size_t operator() (const std::pair<T1, T2> &pair) const
		{
			std::uint64_t hash = 0;
			for(int i=0;i<pair.second.size();i++) {
				hash += pair.second[i].m_id + (std::intptr_t)pair.second[i].m_pool; // Can be anything
			}

			return std::hash<T1>()(pair.first) ^ hash;
		}
	};

} /* util */
