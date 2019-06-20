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
#pragma once

namespace util
{

	//!  Named Type
	/*!
	  This is a naive named type implementation.
	  Its only use is for member variables.
	  For a more detailed implementation see Jonathan Boccara's general purpose implementation
	*/
	template <typename T>
	class NamedType
	{
	public:
		explicit constexpr NamedType(T const& value) : m_value(value)
		{ }

		constexpr T& Get()
		{
			return m_value;
		}
		constexpr T const& Get() const
		{
			return m_value;
		}

		operator T&()
		{
			return m_value;
		}

	private:
		T m_value;
	};

} /* util */