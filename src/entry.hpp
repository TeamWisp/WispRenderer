#pragma once

#define WISP_ENTRY(func) \
int main() \
{ \
	func(); \
	return 0; \
} \
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) \
{ \
	main(); \
}