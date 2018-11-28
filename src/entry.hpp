#pragma once

#define WISP_ENTRY(func)								\
int main()												\
{														\
	return func();										\
}														\
int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)	\
{														\
	return main();										\
}