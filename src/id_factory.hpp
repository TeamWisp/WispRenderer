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

		void MakeIDAvailable(uint64_t unused_id);
		uint64_t GetUnusedID();

	protected:

		uint64_t m_id;
		std::vector<uint64_t> m_unused_ids;
	};
}