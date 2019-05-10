#pragma once
#include <stdint.h>
#include <vector>

namespace wr
{

	class IDFactory
	{
	public:

		IDFactory();
		virtual ~IDFactory() = default;

		IDFactory(IDFactory const &) = delete;
		IDFactory& operator=(IDFactory const &) = delete;
		IDFactory(IDFactory&&) = delete;
		IDFactory& operator=(IDFactory&&) = delete;

		void MakeIDAvailable(std::uint32_t unused_id);
		std::uint32_t GetUnusedID();

	protected:

		std::uint32_t m_id;
		std::vector<uint32_t> m_unused_ids;
	};

} /* wr */