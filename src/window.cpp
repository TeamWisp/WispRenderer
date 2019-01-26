#include "window.hpp"

#include "util/log.hpp"

#include "imgui/imgui.hpp"
#include "imgui/imgui_impl_win32.hpp"

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
#endif

namespace wr
{

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

		RAWINPUTDEVICE Rid[1];
		Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
		Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
		Rid[0].dwFlags = 0;
		Rid[0].hwndTarget = m_handle;

		if (RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])) == FALSE) {
			LOG("NO RAW INPUT");
		}

		ShowCursor(false);

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
		while (PeekMessage(&msg, m_handle, 0, 0, PM_REMOVE))
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

	void Window::SetMouseCallback(MouseCallback callback)
	{
		m_mouse_callback = callback;
	}

	void Window::SetMouseMoveCallback(MouseMoveCallback callback)
	{
		m_mouse_move_callback = callback;
	}

	void Window::SetMouseWheelCallback(MouseWheelCallback callback)
	{
		m_mouse_wheel_callback = callback;
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
		case WM_INPUT:
		{
			UINT dwSize;

			GetRawInputData((HRAWINPUT)l_param, RID_INPUT, NULL, &dwSize,
				sizeof(RAWINPUTHEADER));
			LPBYTE lpb = new BYTE[dwSize];
			if (lpb == NULL)
			{
				return 0;
			}

			if (GetRawInputData((HRAWINPUT)l_param, RID_INPUT, lpb, &dwSize,
				sizeof(RAWINPUTHEADER)) != dwSize)
				OutputDebugString(TEXT("GetRawInputData does not return correct size !\n"));

			RAWINPUT* raw = (RAWINPUT*)lpb;

			if (raw->header.dwType == RIM_TYPEMOUSE)
			{
				int xPosRelative = raw->data.mouse.lLastX;
				int yPosRelative = raw->data.mouse.lLastY;

				if (m_mouse_move_callback) m_mouse_move_callback(xPosRelative, yPosRelative);
			}
			break;
		}
		case WM_DESTROY:
			m_running = false;
			PostQuitMessage(0);
			return 0;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MOUSEMOVE:
			if (m_mouse_callback)
			{
				m_mouse_callback((int)w_param, msg, (int)l_param);
			}
			return 0;
		case WM_KEYDOWN:
		case WM_KEYUP:
			if (m_key_callback)
			{
				m_key_callback((int)w_param, msg, (int)l_param);
			}

			return 0;

		case WM_MOUSEWHEEL:
			if (m_mouse_wheel_callback)
			{
				m_mouse_wheel_callback((int)w_param, msg, (int)l_param);
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

} /* wr */