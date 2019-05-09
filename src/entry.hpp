#pragma once

#define WISP_ENTRY(func)														\
int main()																		\
{																				\
	return func();																\
}																				\
int CALLBACK WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)	\
{																				\
	return main();																\
}