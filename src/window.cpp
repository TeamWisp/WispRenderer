#include "window.hpp"

#include "util/log.hpp"

#include "imgui/imgui.hpp"
#ifndef USE_UWP
#include "imgui/imgui_impl_win32.hpp"
#endif

namespace wr
{

#ifdef USE_UWP
	using namespace Windows::ApplicationModel;
	using namespace Windows::ApplicationModel::Core;
	using namespace Windows::ApplicationModel::Activation;
	using namespace Windows::UI::Core;
	using namespace Windows::UI::Input;
	using namespace Windows::System;
	using namespace Windows::Foundation;
	using namespace Windows::Graphics::Display;

	using Microsoft::WRL::ComPtr;

	InternalUWPApp::InternalUWPApp(Window* app, int width, int height, std::string const& title) :
		window_closed(false),
		window_visible(true),
		initialized(false),
		internal_app(app)
	{
		//Windows::UI::ViewManagement::ApplicationView::PreferredLaunchViewSize = Windows::Foundation::Size(width, height);
		//Windows::UI::ViewManagement::ApplicationView::PreferredLaunchWindowingMode = Windows::UI::ViewManagement::ApplicationViewWindowingMode::PreferredLaunchViewSize;

		app->m_native = CoreWindow::GetForCurrentThread();
		app->m_width = (std::int32_t)app->m_native->Bounds.Width;
		app->m_height = (std::int32_t)app->m_native->Bounds.Height;
	}

	InternalUWPApp::~InternalUWPApp()
	{
		delete internal_app;
	}

	// The first method called when the IFrameworkView is being created.
	void InternalUWPApp::Initialize(CoreApplicationView^ applicationView)
	{
		// Register event handlers for app lifecycle. This example includes Activated, so that we
		// can make the CoreWindow active and start rendering on the window.
		applicationView->Activated +=
			ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &InternalUWPApp::OnActivated);

		CoreApplication::Suspending +=
			ref new EventHandler<SuspendingEventArgs^>(this, &InternalUWPApp::OnSuspending);

		CoreApplication::Resuming +=
			ref new EventHandler<Platform::Object^>(this, &InternalUWPApp::OnResuming);
	}

	// Called when the CoreWindow object is created (or re-created).
	void InternalUWPApp::SetWindow(CoreWindow^ window)
	{
		window->SizeChanged +=
			ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &InternalUWPApp::OnWindowSizeChanged);

		window->VisibilityChanged +=
			ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &InternalUWPApp::OnVisibilityChanged);

		window->Closed +=
			ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &InternalUWPApp::OnWindowClosed);

		DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

		currentDisplayInformation->DpiChanged +=
			ref new TypedEventHandler<DisplayInformation^, Object^>(this, &InternalUWPApp::OnDpiChanged);

		currentDisplayInformation->OrientationChanged +=
			ref new TypedEventHandler<DisplayInformation^, Object^>(this, &InternalUWPApp::OnOrientationChanged);

		DisplayInformation::DisplayContentsInvalidated +=
			ref new TypedEventHandler<DisplayInformation^, Object^>(this, &InternalUWPApp::OnDisplayContentsInvalidated);

		// Styling
		auto title_bar = Windows::UI::ViewManagement::ApplicationView::GetForCurrentView()->TitleBar;

		// Title bar colors. Alpha must be 255.
		title_bar->BackgroundColor = Windows::UI::ColorHelper::FromArgb(255, 82, 176, 67);
		title_bar->ForegroundColor = Windows::UI::ColorHelper::FromArgb(255, 0, 0, 0); // text color
		title_bar->InactiveBackgroundColor = Windows::UI::ColorHelper::FromArgb(255, 24, 52, 20);
		title_bar->InactiveForegroundColor = Windows::UI::ColorHelper::FromArgb(255, 255, 255, 255); // text color

		// Title bar button background colors. Alpha is respected when the view is extended
		// into the title bar (see scenario 2). Otherwise, Alpha is ignored and treated as if it were 255.
		float buttonAlpha = 200;
		title_bar->ButtonBackgroundColor = Windows::UI::ColorHelper::FromArgb(buttonAlpha, 82, 176, 67);
		title_bar->ButtonHoverBackgroundColor = Windows::UI::ColorHelper::FromArgb(buttonAlpha, 73, 158, 60);
		title_bar->ButtonPressedBackgroundColor = Windows::UI::ColorHelper::FromArgb(buttonAlpha, 58, 126, 48);
		title_bar->ButtonInactiveBackgroundColor = Windows::UI::ColorHelper::FromArgb(0, 0, 0, 0);

		// Title bar button foreground colors. Alpha must be 255.
		title_bar->ButtonForegroundColor = Windows::UI::ColorHelper::FromArgb(255, 255, 255, 255);
		title_bar->ButtonHoverForegroundColor = Windows::UI::ColorHelper::FromArgb(255, 255, 255, 255);
		title_bar->ButtonPressedForegroundColor = Windows::UI::ColorHelper::FromArgb(255, 255, 255, 255);
		title_bar->ButtonInactiveForegroundColor = Windows::UI::ColorHelper::FromArgb(0, 0, 0, 0);
	}

	// Initializes scene resources, or loads a previously saved app state.
	void InternalUWPApp::Load(Platform::String^ entryPoint)
	{
	}

	// This method is called after the window becomes active.
	void InternalUWPApp::Run()
	{
		Windows::UI::ViewManagement::ApplicationView::GetForCurrentView()->Title = "Universal Windows Platform";

		while (!window_closed)
		{
			if (window_visible)
			{
				CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

				internal_app->Loop();
			}
			else
			{
				CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
			}
		}
	}

	// Required for IFrameworkView.
	// Terminate events do not cause Uninitialize to be called. It will be called if your IFrameworkView
	// class is torn down while the app is in the foreground.
	void InternalUWPApp::Uninitialize()
	{
	}

	// Application lifecycle event handlers.
	void InternalUWPApp::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
	{
		// Run() won't start until the CoreWindow is activated.
		CoreWindow::GetForCurrentThread()->Activate();

		applicationView->TitleBar->ExtendViewIntoTitleBar = true;

		internal_app->Init();

		initialized = true;
	}

	void InternalUWPApp::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
	{

	}

	void InternalUWPApp::OnResuming(Platform::Object^ sender, Platform::Object^ args)
	{

	}

	// Window event handlers.
	void InternalUWPApp::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
	{
		if (initialized)
		{
			internal_app->OnResize(args->Size.Width, args->Size.Height);
		}
	}

	void InternalUWPApp::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
	{
		window_visible = args->Visible;
	}

	void InternalUWPApp::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
	{
		window_closed = true;
	}

	// DisplayInformation event handlers.
	void InternalUWPApp::OnDpiChanged(DisplayInformation^ sender, Object^ args)
	{

	}

	void InternalUWPApp::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
	{

	}

	void InternalUWPApp::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
	{

	}

#else
	Window::Window(HINSTANCE instance, int show_cmd, std::string const & name, std::uint32_t width, std::uint32_t height)
	{

		WNDCLASSEX wc;
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = &Window::WindowProc;
		wc.cbClsExtra = NULL;
		wc.cbWndExtra = NULL;
		wc.hInstance = instance;
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = name.c_str();
		wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

		if (!RegisterClassEx(&wc))
		{
			LOGC("Failed to register extended window class: ");
		}

		auto window_style = WS_OVERLAPPEDWINDOW;

		if (/*!allow_resizing*/ false)
		{
			window_style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
		}

		RECT client_rect;
		client_rect.left = 0;
		client_rect.right = width;
		client_rect.top = 0;
		client_rect.bottom = height;
		AdjustWindowRectEx(&client_rect, window_style, FALSE, wc.style);

		m_handle = CreateWindowEx(
			wc.style,
			name.c_str(),
			name.c_str(),
			window_style,
			CW_USEDEFAULT, CW_USEDEFAULT,
			client_rect.right - client_rect.left, client_rect.bottom - client_rect.top,
			nullptr,
			nullptr,
			instance,
			nullptr
		);

		if (!m_handle)
		{
			LOGC("Failed to create window." + GetLastError());
		}

		SetWindowLongPtr(m_handle, GWLP_USERDATA, (LONG_PTR)this);

		ShowWindow(m_handle, show_cmd);
		UpdateWindow(m_handle);

		m_running = true;
	}

	Window::Window(HINSTANCE instance, std::string const& name, std::uint32_t width, std::uint32_t height, bool show)
		: Window(instance, show ? SW_SHOWNORMAL : SW_HIDE, name, width, height)
	{

	}

	Window::~Window()
	{
		Stop();
	}

	void Window::PollEvents()
	{
		MSG msg;
		if (PeekMessage(&msg, m_handle, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				m_running = false;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	void Window::Show()
	{
		ShowWindow(m_handle, SW_SHOW);
	}

	void Window::Stop()
	{
		DestroyWindow(m_handle);
	}

	void Window::SetKeyCallback(KeyCallback callback)
	{
		m_key_callback = callback;
	}

	void Window::SetResizeCallback(ResizeCallback callback)
	{
		m_resize_callback = callback;
	}

	bool Window::IsRunning() const
	{
		return m_running;
	}

	std::int32_t Window::GetWidth() const
	{
		RECT r;
		GetClientRect(m_handle, &r);
		return static_cast<std::int32_t>(r.right - r.left);
	}

	std::int32_t Window::GetHeight() const
	{
		RECT r;
		GetClientRect(m_handle, &r);
		return static_cast<std::int32_t>(r.bottom - r.top);
	}

	HWND Window::GetWindowHandle() const
	{
		return m_handle;
	}

	bool Window::IsFullscreen() const
	{
		RECT a, b;
		GetWindowRect(m_handle, &a);
		GetWindowRect(GetDesktopWindow(), &b);
		return (a.left == b.left  &&
			a.top == b.top   &&
			a.right == b.right &&
			a.bottom == b.bottom);
	}

	LRESULT CALLBACK Window::WindowProc(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param)
	{
		extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		if (ImGui_ImplWin32_WndProcHandler(handle, msg, w_param, l_param))
			return true;

		Window* window = (Window*)GetWindowLongPtr(handle, GWLP_USERDATA);
		if (window) return window->WindowProc_Impl(handle, msg, w_param, l_param);

		return DefWindowProc(handle, msg, w_param, l_param);
	}

	LRESULT CALLBACK Window::WindowProc_Impl(HWND handle, UINT msg, WPARAM w_param, LPARAM l_param)
	{
		switch (msg)
		{
		case WM_DESTROY:
			m_running = false;
			PostQuitMessage(0);
			return 0;
		case WM_KEYDOWN:
		case WM_KEYUP:
			if (m_key_callback)
			{
				m_key_callback((int)w_param, msg, (int)l_param);
			}

			return 0;
		case WM_SIZE:
			if (w_param != SIZE_MINIMIZED)
			{
				if (RECT rect; m_resize_callback && GetClientRect(handle, &rect))
				{
					int width = rect.right - rect.left;
					int height = rect.bottom - rect.top;
					m_resize_callback(static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height));
				}
			}
			return 0;
		}

		return DefWindowProc(handle, msg, w_param, l_param);
	}
#endif

} /* wr */