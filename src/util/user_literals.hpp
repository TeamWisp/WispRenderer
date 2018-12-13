#pragma once
constexpr long double operator"" _deg(long double deg)
{
	return DirectX::XMConvertToRadians(deg);
}

constexpr long double operator"" _rad(long double rad)
{
	return DirectX::XMConvertToDegrees(rad);
}