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