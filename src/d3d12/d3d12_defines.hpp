#pragma once

#include "../util/log.hpp"

#define D3DX12_INC d3dx12_rt.h

//! Checks whether the d3d12 object exists before releasing it.
#define SAFE_RELEASE(obj) { if ( obj ) { obj->Release(); obj = NULL; } }

//! Handles a hresult.
#define TRY(result) if (FAILED(result)) { LOGC("An hresult returned a error!. File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__)); }

//! Handles a hresult and outputs a specific message.
#define TRY_M(result, msg) if (FAILED(result)) { LOGC(msg); }

//! This macro is used to name d3d12 resources.
#define NAME_D3D12RESOURCE(r, n) { auto temp = std::string(__FILE__); \
r->SetName(std::wstring(std::wstring(n) + L" (line: " + std::to_wstring(__LINE__) + L" file: " + std::wstring(temp.begin(), temp.end())).c_str()); }

//! This macro is used to name d3d12 resources with a placeholder name. TODO: "Unamed Resource" should be "Unamed [typename]"
#define NAME_D3D12RESOURCE(r) { auto temp = std::string(__FILE__); \
r->SetName(std::wstring(L"Unnamed Resource (line: " + std::to_wstring(__LINE__) + L" file: " + std::wstring(temp.begin(), temp.end())).c_str()); }

// Particular version automatically rounds the alignment to a two power.
template<typename T, typename A>
constexpr inline T SizeAlignTwoPower(T size, A alignment)
{
	return (size + (alignment - 1U)) & ~(alignment - 1U);
}

// Particular version always aligns to the provided alignment
template<typename T, typename A>
constexpr inline T SizeAlignAnyAlignment(T size, A alignment)
{
	return (size / alignment + (size%alignment > 0))*alignment;
}
