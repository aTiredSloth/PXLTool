#ifndef ASSETID_HPP
#define ASSETID_HPP
#include <cstdint>
#include <string>

struct AssetID
{
	std::string Name;
	uint64_t Id;

	bool operator==(const AssetID& other) const
	{
		return Id == other.Id;
	}
};

namespace std
{
	template <>
	struct hash<AssetID>
	{
		std::size_t operator()(const AssetID& u) const noexcept
		{
			// Combine hashes (using a standard bit-shifting and XOR blend)
			return std::hash<uint32_t>{}(u.Id);
		}
	};
}

#endif
