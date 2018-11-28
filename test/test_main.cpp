#include "wisp.hpp"
#include <gtest\gtest.h>

int TestEntry()
{
	auto cl_string = GetCommandLineW();
	int argn = 0;
	wchar_t** cl_string_array = CommandLineToArgvW( cl_string, &argn );

	::testing::InitGoogleTest( &argn, cl_string_array );
	RUN_ALL_TESTS();


	return 0;
}
WISP_ENTRY(TestEntry)