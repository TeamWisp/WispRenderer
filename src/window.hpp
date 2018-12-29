#pragma once

#ifdef USE_UWP
#include <vccorlib.h>
#include <agile.h>
#else
#include <Windows.h>
#endif
#include <functional>

namespace wr
{
	class Window
	{
	public:
		virtual void Init() = 0;
		virtual void Loop() = 0;

		virtual void OnResize(int width, int height) = 0;

		/* Returns the client width */
		std::int32_t GetWidth() const { return m_width; };
		/* Returns the client height */
		std::int32_t GetHeight() const { return m_height; };

#ifdef USE_UWP
		Platform::Agile<Windows::UI::Core::CoreWindow> m_native;
#else
		HWND m_native;
#endif

		std::int32_t m_width;
		std::int32_t m_height;
	};

#ifdef USE_UWP
	ref class InternalUWPApp sealed : public Windows::ApplicationModel::Core::IFrameworkView
	{
	internal:
		InternalUWPApp(Window* app, int width, int height, std::string const& title);

	public:
		virtual ~InternalUWPApp();

		// IFrameworkView methods.
		virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
		virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
		virtual void Load(Platform::String^ entryPoint);
		virtual void Run();
		virtual void Uninitialize();

	protected:
		// Application lifecycle event handlers.
		void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
		void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
		void OnResuming(Platform::Object^ sender, Platform::Object^ args);

		// Window event handlers.
		void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
		void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
		void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);

		// DisplayInformation event handlers.
		void OnDpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
		void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
		void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);

	private:
		Window* internal_app;
		bool window_closed;
		bool window_visible;

		bool initialized;
	};

	template<class T, int width, int height>
	ref class Direct3DApplicationSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
	{
	public:
		virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView()
		{
			return ref new InternalUWPApp(new T(), width, height, "lets go");
		}
	};
#else
	class InternalWin32Window
	{
		using KeyCallback = std::function<void(int key, int action, int mods)>;
		using ResizeCallback = std::function<void(std::uint32_t width, std::uint32_t height)>;
	public:
		/*!
		* @param instance A handle to the current instance of the application.
		* @param name Window title.
		* @param width Initial window width.
		* @param height Initial window height.
		* @param show Controls whether the window will be shown. Default is true.
		*/
		InternalWin32Window(HINSTANCE instance, std::string const& name, std::uint32_t width, std::uint32_t height, bool show = true);
		InternalWin32Window(HINSTANCE instance, int show_cmd, std::string const& name, std::uint32_t width, std::uint32_t height);
		~InternalWin32Window();

		InternalWin32Window(const InternalWin32Window	&) = delete;
		InternalWin32Window& operator=(const InternalWin32Window&) = delete;
		InternalWin32Window(InternalWin32Window&&) = delete;
		InternalWin32Window& operator=(InternalWin32Window&&) = delete;

		/*! Handles window events. Should be called every frame */
		void PollEvents();
		/*! Shows the window if it was hidden */
		void Show();
		/*! Requests to close the window */
		void Stop();

		/*! Used to set the key callback function */
		void SetKeyCallback(KeyCallback callback);
		/*! Used to set the resize callback function */
		void SetResizeCallback(ResizeCallback callback);

		/*! Returns whether the application is running. (used for the main loop) */
		bool IsRunning() const;
		/* Returns the client width */
		std::int32_t GetWidth() const;
		/* Returns the client height */
		std::int32_t GetHeight() const;
		/*! Returns the native window handle (HWND)*/
		HWND GetWindowHandle() const;
		/*! Checks whether the window is fullscreen */
		bool IsFullscreen() const;

	private:
		/*! WindowProc that calls `WindowProc_Impl` */
		static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
		/*! Main WindowProc function */
		LRESULT CALLBACK WindowProc_Impl(HWND, UINT, WPARAM, LPARAM);

		KeyCallback m_key_callback;
		ResizeCallback m_resize_callback;

		bool m_running;
		HWND m_handle;
	};
#endif

} /* wr */