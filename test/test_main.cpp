#include "wisp.hpp"

#include <gtest\gtest.h>

TEST( sample_test, int_equ )
{
	int i = 4;
	EXPECT_EQ( i, 4 );
}

TEST( sample_test, int_neq )
{
	int i = 4;
	EXPECT_NE( i, 5 );
}

//TEST( sample_test, int_ge )
//{
//	int i = 4;
//	EXPECT_GE( i, 5 );
//}

int TestEntry()
{
	auto cl_string = GetCommandLineW();
	int argn = 0;
	wchar_t** cl_string_array = CommandLineToArgvW( cl_string, &argn );

	::testing::InitGoogleTest( &argn, cl_string_array );
	


	return RUN_ALL_TESTS();
}
WISP_ENTRY( TestEntry )