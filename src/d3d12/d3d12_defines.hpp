#pragma once

#include "../util/log.hpp"

#define D3DX12_INC d3dx12_rt.h

inline std::string HResultToString(HRESULT hr)
{
	if (FACILITY_WINDOWS == HRESULT_FACILITY(hr))
		hr = HRESULT_CODE(hr);
	TCHAR* szErrMsg;

	if (FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&szErrMsg, 0, NULL) != 0)
	{
		return(szErrMsg);
		LocalFree(szErrMsg);
	}
	else
		return(std::string("[Could not find a description for error # %#x.", hr));
}

//! Checks whether the d3d12 object exists before releasing it.
#define SAFE_RELEASE(obj) { if ( obj ) { obj->Release(); obj = NULL; } }

//! Handles a hresult.
#define TRY(result) if (FAILED(result)) { LOGC("An hresult returned a error!. File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__) + " HRResult: " +  HResultToString(result)); }

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
