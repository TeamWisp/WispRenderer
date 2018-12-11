#pragma once
constexpr long double operator"" _deg(long double deg)
{
	return deg * 3.14159265358979323846 / 180.0;
}

constexpr long double operator"" _rad(long double rad)
{
	return rad * 180.0 / 3.14159265358979323846;
}