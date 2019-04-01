#pragma once

namespace util
{

	struct PairHash {
		template <class T1, class T2>
		std::size_t operator() (const std::pair<T1, T2> &pair) const
		{
			int hash = 0;
			for(int i=0;i<pair.second.size();i++) {
				hash += pair.second[i].m_id + (int)pair.second[i].m_pool; // Can be anything
			}

			return std::hash<T1>()(pair.first) ^ hash;
		}
	};

} /* util */
