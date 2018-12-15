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
		explicit constexpr NamedType(T const& value) : m_value(value) {}
    
		constexpr T& Get() { return m_value; }
		constexpr T const& Get() const {return m_value; }

		operator T&() { return m_value; }
    
	private:
		T m_value;
	};

} /* fluent */
