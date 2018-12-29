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

#ifdef USE_UWP
#define CREATE_GAME(type, width, height, title) [Platform::MTAThread]\
int main(Platform::Array<Platform::String^>^) \
{ \
	auto direct3DApplicationSource = ref new Direct3DApplicationSource<type, width, height>(); \
	Windows::ApplicationModel::Core::CoreApplication::Run(direct3DApplicationSource); \
	return 0; \
}
#else
#define CREATE_GAME(type, width, height, title) \
int main() \
{ \
	auto app = new type(); \
	auto internal_app = new InternalWin32App(app, width, height, title); \
	internal_app->Run(); \
	delete internal_app; \
	return 0; \
}
#endif