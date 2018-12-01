#pragma once

#include <cstdint>
#include <vector>
#include <optional>

namespace util
{
	inline std::uint64_t IndexFromBit(std::uint64_t frame)
	{
		return frame / (8 * 8);
	}

	inline std::uint64_t OffsetFromBit(std::uint64_t frame)
	{
		return frame % (8 * 8);
	}

	inline void SetPage(std::vector<uint64_t> & bitmap, std::uint64_t frame)
	{
		const std::uint64_t idx = IndexFromBit(frame);
		const std::uint64_t off = OffsetFromBit(frame);
		bitmap[idx] |= (1Ui64 << off);
	}

	inline void ClearPage(std::vector<uint64_t> & bitmap, std::uint64_t frame)
	{
		const std::uint64_t idx = IndexFromBit(frame);
		const std::uint64_t off = OffsetFromBit(frame);
		bitmap[idx] &= ~(1Ui64 << off);
	}

	inline bool TestPage(std::vector<uint64_t> const & bitmap, std::uint64_t frame)
	{
		const std::uint64_t idx = IndexFromBit(frame);
		const std::uint64_t off = OffsetFromBit(frame);

		return (bitmap[idx] & (1Ui64 << off));
	}

	inline std::optional<std::uint64_t> FindFreePage(std::vector<std::uint64_t> const & bitmap, std::size_t const frame_count, std::uint64_t needed_frames)
	{
		bool counting = false;
		bool found = false;

		std::uint64_t start_frame = 0;
		std::uint64_t free_frames = 0;

		//Go through all all pages to find free ones.
		//Because we're storing the data in a single bit we can compare an entire int64 at once to speed things up
		for (std::uint64_t i = 0; i < bitmap.size(); ++i)
		{
			//Check 64 pages at once
			if (bitmap[i] != 0Ui64)
			{
				//At least a single page is free, so check all 64 pages
				for (std::uint64_t j = 0; j < 64; ++j)
				{
					//If the current page being checked is larger than the amount of available pages, break
					if (i * 64 + j >= frame_count)
					{
						break;
					}

					//Set bit corresponding to page being checked in temporary variable
					std::uint64_t to_test = 1Ui64 << j;

					//Run an 'and' against the temporary variable to see if the page is available
					if ((bitmap[i] & to_test))
					{
						//Is this the first free page found?
						if (counting)
						{
							//Not the first free page, so increment the counter of consecutive free pages
							free_frames++;
							//Have we found the nessecary amount of free pages? If so, indicate that and break
							if (free_frames == needed_frames)
							{
								found = true;
								break;
							}
						}
						else
						{
							//This is the first free page
							if (needed_frames == 1)
							{
								//We only need a single page, so store the number of the page and break
								found = true;
								start_frame = i * 64 + j;
								break;
							}
							else
							{
								//We need multiple pages, so store the number of this page as the start page and continue
								start_frame = i * 64 + j;
								counting = true;
								free_frames = 1;
							}
						}
					}
					else
					{
						//The page wasn't free, so stop counting.
						counting = 0;
						free_frames = 0;
					}
				}
			}
			else
			{
				//All 64 pages are occupied, so stop counting
				counting = false;
				free_frames = 0;
			}

			//Have we found enough free pages? If so, break
			if (found == true)
			{
				break;
			}
		}

		return start_frame;
	}
} /* util */
