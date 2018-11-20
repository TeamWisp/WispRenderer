#pragma once

#define WISP_ENTRY(func) \
int main() \
{ \
	func(); \
	return 0; \
} \
int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int) \
{ \
	main(); \
	return 0; \
}