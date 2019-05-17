#pragma once

#include <vector>
#include <string>
#include <type_traits>
#include <stack>
#include <deque>
#include <future>
#include <any>

#include "../util/thread_pool.hpp"
#include "../util/delegate.hpp"
#include "../renderer.hpp"
#include "../platform_independend_structs.hpp"
#include "../settings.hpp"
#include "../d3d12/d3d12_settings.hpp"
#include "../structs.hpp"
#include "../wisprenderer_export.hpp"

#ifndef _DEBUG
#define FG_MAX_PERFORMANCE
#endif

#define EXPAND(x) x // Because msvc handles the preprocessor differently
#define FG_DEPS(N, ...) EXPAND(FG_DEPS##N(__VA_ARGS__))
#define FG_DEPS1(A) { typeid(A) }
#define FG_DEPS2(A, B) { typeid(A), typeid(B) }
#define FG_DEPS3(A, B, C) { typeid(A), typeid(B), typeid(C) }
#define FG_DEPS4(A, B, C, D) { typeid(A), typeid(B), typeid(C), typeid(D) }
#define FG_DEPS5(A, B, C, D, E) { typeid(A), typeid(B), typeid(C), typeid(D), typeid(E) }
#define FG_DEPS6(A, B, C, D, E, F) { typeid(A), typeid(B), typeid(C), typeid(D), typeid(E), typeid(F) }

namespace wr
{
	enum class RenderTaskType
	{
		DIRECT,
		COMPUTE,
		COPY
	};

	enum class CPUTextureType
	{
		PIXEL_DATA,
		DEPTH_DATA
	};

	struct CPUTextures
	{
		std::optional<CPUTexture> pixel_data = std::nullopt;
		std::optional<CPUTexture> depth_data = std::nullopt;
	};

	//! Typedef for the render task handle.
	using RenderTaskHandle = std::uint32_t;

	// Forward declarations.
	class FrameGraph;

	/*! Structure that describes a render task */
	/*!
		All non default initialized member variables should be fully initialized to prevent undifined behaviour.
	*/
	struct RenderTaskDesc
	{
		// Typedef the function pointer types to keep the code readable.
		using setup_func_t =   util::Delegate<void(RenderSystem&, FrameGraph&, RenderTaskHandle, bool)>;
		using execute_func_t = util::Delegate<void(RenderSystem&, FrameGraph&, SceneGraph&, RenderTaskHandle)>;
		using destroy_func_t = util::Delegate<void(FrameGraph&, RenderTaskHandle, bool)>;

		/*! The type of the render task.*/
		RenderTaskType m_type = RenderTaskType::DIRECT;
		/*! The function pointers for the task.*/
		setup_func_t m_setup_func;
		execute_func_t m_execute_func;
		destroy_func_t m_destroy_func;

		/*! The properties for the render target this task renders to. If this is `std::nullopt` no render target will be created. */
		std::optional<RenderTargetProperties> m_properties;

		bool m_allow_multithreading = true;
	};

	//!  Frame Graph 
	/*!
	  The Frame Graph is responsible for managing all tasks the renderer should perform.
	  The idea is you can add tasks to the scene graph and when you call `RenderSystem::Render` it will run the tasks added.
	  It will not just run tasks but will also assign command lists and render targets to the tasks.
	  The Frame Graph is also capable of mulithreaded execution.
	  It will split the command lists that are allowed to be multithreaded on X amount of threads specified in `settings.hpp`
	*/
	class FrameGraph
	{
		// Obtain the type definitions from `RenderTaskDesc` to keep the code readable.
		using setup_func_t   = RenderTaskDesc::setup_func_t;
		using execute_func_t = RenderTaskDesc::execute_func_t;
		using destroy_func_t = RenderTaskDesc::destroy_func_t;
	public:

		//! Constructor.
		/*!
			This constructor is able to reserve space for render tasks.
			This works by calling `std::vector::reserve`.
			\param num_reserved_tasks Amount of tasks we should reserve space for.
		*/
		FrameGraph(std::size_t num_reserved_tasks = 1) :
			m_render_system(nullptr),
			m_num_tasks(0),
			m_thread_pool(new util::ThreadPool(settings::num_frame_graph_threads)),
			m_uid(GetFreeUID())
		{
			// lambda to simplify reserving space.
			auto reserve = [num_reserved_tasks](auto v) { v.reserve(num_reserved_tasks); };

			// Reserve space for all vectors.
			reserve(m_setup_funcs);
			reserve(m_execute_funcs);
			reserve(m_destroy_funcs);
			reserve(m_cmd_lists);
			reserve(m_render_targets);
			reserve(m_data);
			reserve(m_data_type_info);
#ifndef FG_MAX_PERFORMANCE
			reserve(m_dependencies);
#endif
			reserve(m_types);
			reserve(m_rt_properties);
			m_settings = decltype(m_settings)(num_reserved_tasks, std::nullopt); // Resizing so I can initialize it with null since this is an optional value.
			m_futures.resize(num_reserved_tasks); // std::thread doesn't allow me to reserve memory for the vector. Hence I'm resizing.
		}

		//! Destructor
		/*!
			This destructor destroys all the task data and the thread pool.
			If you want to reuse the frame graph I recommend calling `FrameGraph::Destroy`
		*/
		~FrameGraph()
		{
			delete m_thread_pool;
			Destroy();
		}

		FrameGraph(const FrameGraph&) = delete;
		FrameGraph(FrameGraph&&)      = delete;

		FrameGraph& operator=(const FrameGraph&) = delete;
		FrameGraph& operator=(FrameGraph&&) 	 = delete;

		//! Setup the render tasks
		/*!
			Calls all setup function pointers and obtains the required render targets and command lists.
			It is recommended to try to avoid calling this during runtime because it can cause stalls if the setup functions are expensive.
			\param render_system The render system we want to use for rendering.
		*/
		inline void Setup(RenderSystem& render_system)
		{
			bool is_valid = Validate();

			if (!is_valid)
			{
				LOGE("Framegraph validation failed. Aborting setup.");
				return;
			}

			// Resize these vectors since we know the end size already.
			m_cmd_lists.resize(m_num_tasks);
			m_should_execute.resize(m_num_tasks, true); // All tasks should execute by default.
			m_render_targets.resize(m_num_tasks);
			m_futures.resize(m_num_tasks);
			m_render_system = &render_system;

			if constexpr (settings::use_multithreading)
			{
				for (decltype(m_num_tasks) i = 0; i < m_num_tasks; ++i)
				{
					// Get the proper command list from the render system.
					switch (m_types[i])
					{
					case RenderTaskType::DIRECT:
						m_cmd_lists[i] = render_system.GetDirectCommandList(d3d12::settings::num_back_buffers);
						break;
					case RenderTaskType::COMPUTE:
						m_cmd_lists[i] = render_system.GetComputeCommandList(d3d12::settings::num_back_buffers);
						break;
					case RenderTaskType::COPY:
						m_cmd_lists[i] = render_system.GetCopyCommandList(d3d12::settings::num_back_buffers);
						break;
					default:
						break;
					}

					// Get a render target from the render system.
					m_render_targets[i] = render_system.GetRenderTarget(m_rt_properties[i].value());
				}

				Setup_MT_Impl(render_system);
			}
			else
			{
				// Itterate over all the tasks.
				for (decltype(m_num_tasks) i = 0; i < m_num_tasks; ++i)
				{
					// Get the proper command list from the render system.
					switch (m_types[i])
					{
					case RenderTaskType::DIRECT:
						m_cmd_lists[i] = render_system.GetDirectCommandList(d3d12::settings::num_back_buffers);
						break;
					case RenderTaskType::COMPUTE:
						m_cmd_lists[i] = render_system.GetComputeCommandList(d3d12::settings::num_back_buffers);
						break;
					case RenderTaskType::COPY:
						m_cmd_lists[i] = render_system.GetCopyCommandList(d3d12::settings::num_back_buffers);
						break;
					default:
						break;
					}

					// Get a render target from the render system.
					m_render_targets[i] = render_system.GetRenderTarget(m_rt_properties[i].value());

					// Call the setup function pointer.
					m_setup_funcs[i](render_system, *this, i, false);
				}
			}
		}

		/*! Execute all render tasks */
		/*!
			For every render task call the setup function pointers and tell the render system we started a render task of a certain type.
			\param render_system The render system we want to use for rendering.
			\param scene_graph The scene graph we want to render.
		*/
		inline void Execute(RenderSystem& render_system, SceneGraph& scene_graph)
		{
			ResetOutputTexture();

			// Check if we need to disable some tasks
			while (!m_should_execute_change_request.empty())
			{
				auto front = m_should_execute_change_request.front();
				m_should_execute[front.first] = front.second;
				m_should_execute_change_request.pop();
			}

			if constexpr (settings::use_multithreading)
			{
				Execute_MT_Impl(render_system, scene_graph);
			}
			else
			{
				Execute_ST_Impl(render_system, scene_graph);
			}
		}

		/*! Resize all render tasks */
		/*!
			This function calls resize all render tasks to a specific width and height.
			The width and height parameters should be the output size.
			Please note this function calls Destroy than setup with the resize boolean set to true.
		*/
		inline void Resize(RenderSystem & render_system, std::uint32_t width, std::uint32_t height)
		{
			for (decltype(m_num_tasks) i = 0; i < m_num_tasks; ++i)
			{
				WaitForCompletion(i);

				m_destroy_funcs[i](*this, i, true);

				if (!m_rt_properties[i].value().m_is_render_window)
				{
					render_system.ResizeRenderTarget(&m_render_targets[i],
						static_cast<std::uint32_t>(width * m_rt_properties[i].value().m_resolution_scale.Get()),
						static_cast<std::uint32_t>(height * m_rt_properties[i].value().m_resolution_scale.Get()));
				}

				m_setup_funcs[i](render_system, *this, i, true);
			}
		}

		/*! Get Resolution scale of specified Render Task */
		/*!
			Checks if specified RenderTask has valid properties and returns it's resolution scalar.
		*/
		[[nodiscard]] inline const float GetRenderTargetResolutionScale(RenderTaskHandle handle) const
		{
			if (m_rt_properties[handle].has_value())
			{
				return m_rt_properties[handle].value().m_resolution_scale.Get();
			}
			else
			{
				LOGW("Error: GetResolutionScale tried accessing invalid data!")
			}
			return 1.0f;
		}

		/*! Destroy all tasks */
		/*!
			Calls all destroy functions and release any allocated data.
		*/
		void Destroy()
		{
			// Allow the tasks to safely deallocate the stuff they allocated.
			for (decltype(m_num_tasks) i = 0; i < m_num_tasks; ++i)
			{
				WaitForCompletion(i);
				m_destroy_funcs[i](*this, i, false);
			}

			// Make sure we free the data objects we allocated.
			for (auto& data : m_data)
			{
				delete data;
			}

			for (auto& cmd_list : m_cmd_lists)
			{
				m_render_system->DestroyCommandList(cmd_list);
			}

			// Reset all members in the case of the user wanting to reuse this frame graph after `FrameGraph::Destroy`.
			m_setup_funcs.clear();
			m_execute_funcs.clear();
			m_destroy_funcs.clear();
			m_cmd_lists.clear();
			m_render_targets.clear();
			m_data.clear();
			m_data_type_info.clear();
			m_settings.clear();
#ifndef FG_MAX_PERFORMANCE
			m_dependencies.clear();
#endif
			m_types.clear();
			m_rt_properties.clear();
			m_futures.clear();

			m_num_tasks = 0;
		}

		/* Stall the current thread until the render task has finished. */
		inline void WaitForCompletion(RenderTaskHandle handle)
		{
			// If we are not allowed to use multithreading let the compiler optimize this away completely.
			if constexpr (settings::use_multithreading)
			{
				if (auto& future = m_futures[handle]; future.valid())
				{
					future.wait();
				}
			}
		}

		/*! Wait for a previous task. */
		/*!
			This function loops over all tasks and checks whether it has the same type information as the template variable.
			If a task was found it waits for it.
			If no task is found with the type specified a nullptr will be returned and a error message send to the logging system.
			The template parameter should be a Data struct of a the task you want to wait for.
		*/
		template<typename T>
		inline void WaitForPredecessorTask()
		{
			static_assert(std::is_class<T>::value ||
				std::is_floating_point<T>::value ||
				std::is_integral<T>::value,
				"The template variable should be a class, struct, floating point value or a integral value.");
			static_assert(!std::is_pointer<T>::value,
				"The template variable type should not be a pointer. Its implicitly converted to a pointer.");

			for (decltype(m_num_tasks) i = 0; i < m_num_tasks; i++)
			{
				if (typeid(T) == m_data_type_info[i])
				{
					WaitForCompletion(i);
					return;
				}
			}

			LOGC("Failed to find predecessor data! Please check your task order.");
			return;
		}

		/*! Get the data of a task. (Modifyable) */
		/*!
			The template variable is used to specify the type of the data structure.
			\param handle The handle to the render task. (Given by the `Setup`, `Execute` and `Destroy` functions)
		*/
		template<typename T = void*>
		[[nodiscard]] inline auto & GetData(RenderTaskHandle handle) const
		{
			static_assert(std::is_class<T>::value ||
				std::is_floating_point<T>::value ||
				std::is_integral<T>::value,
				"The template variable should be a class, struct, floating point value or a integral value.");
			static_assert(!std::is_pointer<T>::value,
				"The template variable type should not be a pointer. Its implicitly converted to a pointer.");

			return *static_cast<T*>(m_data[handle]);
		}

		/*! Get the data of a previously ran task. (Constant) */
		/*!
			This function loops over all tasks and checks whether it has the same type information as the template variable.
			If no task is found with the type specified a nullptr will be returned and a error message send to the logging system.
			\param handle The handle to the render task. (Given by the `Setup`, `Execute` and `Destroy` functions)
		*/
		template<typename T>
		[[nodiscard]] inline auto const & GetPredecessorData()
		{
			static_assert(std::is_class<T>::value ||
				std::is_floating_point<T>::value ||
				std::is_integral<T>::value,
				"The template variable should be a class, struct, floating point value or a integral value.");
			static_assert(!std::is_pointer<T>::value,
				"The template variable type should not be a pointer. Its implicitly converted to a pointer.");

			for (decltype(m_num_tasks) i = 0; i < m_num_tasks; i++)
			{
				if (typeid(T) == m_data_type_info[i])
				{
					WaitForCompletion(i);

					return *static_cast<T*>(m_data[i]);
				}
			}

			LOGC("Failed to find predecessor data! Please check your task order.")
			return *static_cast<T*>(nullptr);
		}

		/*! Get the render target of a previously ran task. (Constant) */
		/*!
			This function loops over all tasks and checks whether it has the same type information as the template variable.
			If no task is found with the type specified a nullptr will be returned and a error message send to the logging system.
		*/
		template<typename T>
		[[nodiscard]] inline RenderTarget* GetPredecessorRenderTarget()
		{
			static_assert(std::is_class<T>::value,
				"The template variable should be a class or struct.");
			static_assert(!std::is_pointer<T>::value,
				"The template variable type should not be a pointer. Its implicitly converted to a pointer.");

			for (decltype(m_num_tasks) i = 0; i < m_num_tasks; i++)
			{
				if (typeid(T) == m_data_type_info[i])
				{
					WaitForCompletion(i);

					return m_render_targets[i];			
				}
			}

			LOGC("Failed to find predecessor render target! Please check your task order.");
			return nullptr;
		}

		/*! Get the command list of a task. */
		/*!
			The template variable allows you to cast the command list to a "non platform independent" different type. For example a `D3D12CommandList`.
			\param handle The handle to the render task. (Given by the `Setup`, `Execute` and `Destroy` functions)
		*/
		template<typename T = CommandList>
		[[nodiscard]] inline auto GetCommandList(RenderTaskHandle handle) const
		{
			static_assert(std::is_class<T>::value || std::is_void<T>::value,
				"The template variable should be a void, class or struct.");
			static_assert(!std::is_pointer<T>::value,
				"The template variable type should not be a pointer. Its implicitly converted to a pointer.");

			return static_cast<T*>(m_cmd_lists[handle]);
		}

		/*! Get the command list of a previously ran task. */
		/*!
			The function allows the user to get a command list from another render task. These command lists are not meant
			to be used as they could be closed or in flight. This function was created only so that ray tracing tasks could get
			the heap from the acceleration structure command list.
		*/
		template<typename T>
		[[nodiscard]] inline wr::CommandList* GetPredecessorCommandList()
		{
			static_assert(std::is_class<T>::value,
				"The template variable should be a void, class or struct.");
			static_assert(!std::is_pointer<T>::value,
				"The template variable type should not be a pointer. Its implicitly converted to a pointer.");

			for (decltype(m_num_tasks) i = 0; i < m_num_tasks; i++)
			{
				if (typeid(T) == m_data_type_info[i])
				{
					WaitForCompletion(i);

					return m_cmd_lists[i];
				}
			}

			LOGC("Failed to find predecessor command list! Please check your task order.");
			return nullptr;
		}

		template<typename T>
		[[nodiscard]] std::vector<T*> GetAllCommandLists()
		{
			std::vector<T*> retval;
			retval.reserve(m_num_tasks);

			// TODO: Just return the fucking vector as const ref.
			for (decltype(m_num_tasks) i = 0; i < m_num_tasks; i++)
			{
				// Don't return command lists from tasks that don't require to be executed.
				if (!m_should_execute[i])
				{
					continue;
				}

				WaitForCompletion(i);
				retval.push_back(static_cast<T*>(m_cmd_lists[i]));
			}

			return retval;
		}

		/*! Get the render target of a task. */
		/*!
			The template variable allows you to cast the render target to a "non platform independent" different type. For example a `D3D12RenderTarget`.
			\param handle The handle to the render task. (Given by the `Setup`, `Execute` and `Destroy` functions)
		*/
		template<typename T = RenderTarget>
		[[nodiscard]] inline auto GetRenderTarget(RenderTaskHandle handle) const
		{
			static_assert(std::is_class<T>::value || std::is_void<T>::value,
				"The template variable should be a void, class or struct.");
			static_assert(!std::is_pointer<T>::value,
				"The template variable type should not be a pointer. Its implicitly converted to a pointer.");

			return static_cast<T*>(m_render_targets[handle]);
		}

		/*! Check if this frame graph has a task. */
		/*!
			This checks if the frame graph has the task that has been given as the template variable.
		*/
		template<typename T>
		inline bool HasTask() const
		{
			return GetHandleFromType<T>().has_value();
		}

		/*! Validates the frame graph for correctness */
		/*!
			This function uses the dependencies to check whether the frame graph is constructed properly by the user.
			Note: This function only works when `FG_MAX_PERFORMANCE` is defined.
		*/
		bool Validate()
		{
			bool result = true;
#ifndef FG_MAX_PERFORMANCE

			// Loop over all the tasks.
			for (decltype(m_num_tasks) handle = 0; handle < m_num_tasks; ++handle)
			{
				// Loop over the task's dependencies.
				for (auto dependency : m_dependencies[handle])
				{
					bool found_dependency = false;

					// Loop over the predecessor tasks.
					for (decltype(m_num_tasks) prev_handle = 0; prev_handle < handle; ++prev_handle)
					{
						const auto& task_type_info = m_data_type_info[prev_handle].get();
						if (task_type_info == dependency.get())
						{
							found_dependency = true;
						}
					}

					if (!found_dependency)
					{
						LOGW("Framegraph validation: Failed to find dependency {}", dependency.get().name());
						result = false;
					}
				}
			}
#endif

			return result;
		}

		/*! Add a task to the Frame Graph. */
		/*!
			This creates a new render task based on a description.
			The dependencies parameters can contain a list of typeid's of render tasks this task depends on.
			You can use the FG_DEPS macro as followed: `AddTask<desc, FG_DEPS(OtherTaskData)>`
			\param desc A description of the render task.
		*/
		template<typename T>
		inline void AddTask(RenderTaskDesc& desc, std::vector<std::reference_wrapper<const std::type_info>> dependencies = {})
		{
			static_assert(std::is_class<T>::value ||
				std::is_floating_point<T>::value ||
				std::is_integral<T>::value,
				"The template variable should be a class, struct, floating point value or a integral value.");
			static_assert(std::is_default_constructible<T>::value,
				"The template variable is not default constructible or nothrow consructible!");
			static_assert(!std::is_pointer<T>::value,
				"The template variable type should not be a pointer. Its implicitly converted to a pointer.");

			m_setup_funcs.emplace_back(desc.m_setup_func);
			m_execute_funcs.emplace_back(desc.m_execute_func);
			m_destroy_funcs.emplace_back(desc.m_destroy_func);
#ifndef FG_MAX_PERFORMANCE
			m_dependencies.emplace_back(dependencies);
#endif
			m_settings.resize(m_num_tasks + 1ull);
			m_types.emplace_back(desc.m_type);
			m_rt_properties.emplace_back(desc.m_properties);
			m_data.emplace_back(new (std::nothrow) T());
			m_data_type_info.emplace_back(typeid(T));

			// If we are allowed to do multithreading place the task in the appropriate vector
			if constexpr (settings::use_multithreading)
			{
				if (desc.m_allow_multithreading)
				{
					m_multi_threaded_tasks.emplace_back(m_num_tasks);
				}
				else
				{
					m_single_threaded_tasks.emplace_back(m_num_tasks);
				}
			}

			m_num_tasks++;
		}

		/*! Return the frame graph's unique id.*/
		[[nodiscard]] const std::uint64_t GetUID() const
		{
			return m_uid;
		};

		/*! Return the cpu texture. */
		[[nodiscard]] CPUTextures const & GetOutputTexture() const
		{
			return m_output_cpu_textures;
		}

		/*! Save a render target to disc */
		/*
			Tells the render system to save a render target to disc as a image.
			\param index The index of the render target from the task you want to save.
		*/
		template<typename T>
		void SaveTaskToDisc(std::string const & path, int index = 0)
		{
			auto handle = GetHandleFromType<T>();

			if (handle.has_value())
			{
				m_render_system->RequestRenderTargetSaveToDisc(path, m_render_targets[handle.value()], index);
			}
			else
			{
				LOGW("Failed to save render task to disc, Task was not found.");
			}
		}

		/*! Set the cpu texture's data. */
		void SetOutputTexture(const CPUTexture& output_texture, CPUTextureType type)
		{
			switch (type)
			{
			case wr::CPUTextureType::PIXEL_DATA:
				if (m_output_cpu_textures.pixel_data != std::nullopt)
					LOGW("Warning: CPU texture pixel data is written to more than once a frame!")

				// Save the pixel data
				m_output_cpu_textures.pixel_data = output_texture;
				break;

			case wr::CPUTextureType::DEPTH_DATA:
				if (m_output_cpu_textures.depth_data != std::nullopt)
					LOGW("Warning: CPU texture depth data is written to more than once a frame!")

				// Save the depth data
				m_output_cpu_textures.depth_data = output_texture;
				break;

			default:
				// Should never happen
				LOGC("Invalid CPU texture type supplied!")
				break;
			}
		}

		/*! Enable or disable execution of a task. */
		/*!
			Note that this function is not thread safe.
		*/
		inline void SetShouldExecute(RenderTaskHandle handle, bool value)
		{
			m_should_execute_change_request.emplace(std::make_pair(handle, value));
		}
		
		/*! Update the settings of a task. */
		/*!
			This is used to update settings of a render task.
			This must ge called BEFORE `FrameGraph::Setup` or `RenderSystem::Render`.
		*/
		template<typename T>
		inline void UpdateSettings(std::any settings)
		{
			auto handle = GetHandleFromType<T>();

			if (handle.has_value())
			{
				m_settings[handle.value()] = settings;
			}
			else
			{
				LOGW("Failed to update settings, Could not find render task");
			}
		}

		/*! Gives you the settings of a task by handle. */
		/*!
			This gives you the settings for a render task casted to `R`.
			Meant to be used for INSIDE the tasks.
			The return value can be a nullptr.
			\tparam T The render task data type used for identification.
			\tparam R The type of the settings object.

		*/
		template<typename T, typename R>
		[[nodiscard]] inline R GetSettings() const
		{
			static_assert(std::is_class<T>::value ||
				std::is_floating_point<T>::value ||
				std::is_integral<T>::value,
				"The first template variable should be a class, struct, floating point value or a integral value.");

			for (decltype(m_num_tasks) i = 0; i < m_num_tasks; i++)
			{
				if (typeid(T) == m_data_type_info[i])
				{
					try {
						return std::any_cast<R>(m_settings[i].value());
					}
					catch (const std::bad_any_cast & e) {
						LOGW("A task settings requested failed to cast to T. ({})", e.what());
						return R();
					}
				}
			}

			LOGC("Failed to find task settings! Does your frame graph contain this task?");
			return R();
		}

		/*! Gives you the settings of a task by handle. */
		/*!
			This gives you the settings for a render task casted to `T`.
			Meant to be used for INSIDE the tasks.
			The return value can be a nullptr.
		*/
		template<typename T>
		[[nodiscard]] inline T GetSettings(RenderTaskHandle handle) const
		{
			static_assert(std::is_class<T>::value ||
				std::is_floating_point<T>::value ||
				std::is_integral<T>::value,
				"The template variable should be a class, struct, floating point value or a integral value.");

			try {
				return std::any_cast<T>(m_settings[handle].value());
			}
			catch (const std::bad_any_cast & e) {
				LOGW("A task settings requested failed to cast to T. ({})", e.what());
			}
			
			return T();
		}

		/*! Resets the CPU texture data for this frame. */
		void ResetOutputTexture()
		{
			// Frame has been rendered, allow a task to write to the CPU texture in the next frame
			m_output_cpu_textures.pixel_data = std::nullopt;
			m_output_cpu_textures.depth_data = std::nullopt;
		}

	private:

		/*! Get the handle from a task by data type */
		/* This function is zero overhead with GCC-8.2, -03 */
		template<typename T>
		inline std::optional<RenderTaskHandle> GetHandleFromType() const
		{
			for (decltype(m_num_tasks) i = 0; i < m_num_tasks; ++i)
			{
				if (m_data_type_info[i].get() == typeid(T))
				{
					return i;
				}
			}

			return std::nullopt;
		}

		/*! Setup tasks multi threaded */
		inline void Setup_MT_Impl(RenderSystem& render_system)
		{
			// Multithreading behaviour
			for (const auto handle : m_multi_threaded_tasks)
			{
				m_futures[handle] = m_thread_pool->Enqueue([this, handle, &render_system]
				{
					m_setup_funcs[handle](render_system, *this, handle, false);
				});
			}

			// Singlethreading behaviour
			for (const auto handle : m_single_threaded_tasks)
			{
				m_setup_funcs[handle](render_system, *this, handle, false);
			}
		}

		/*! Execute tasks multi threaded */
		inline void Execute_MT_Impl(RenderSystem& render_system, SceneGraph& scene_graph)
		{
			// Multithreading behaviour
			for (const auto handle : m_multi_threaded_tasks)
			{
				// Skip this task if it doesn't need to be executed
				if (!m_should_execute[handle])
				{
					continue;
				}

				m_futures[handle] = m_thread_pool->Enqueue([this, handle, &render_system, &scene_graph]
				{
					ExecuteSingleTask(render_system, scene_graph, handle);
				});
			}

			// Singlethreading behaviour
			for (const auto handle : m_single_threaded_tasks)
			{
				// Skip this task if it doesn't need to be executed
				if (!m_should_execute[handle])
				{
					continue;
				}

				ExecuteSingleTask(render_system, scene_graph, handle);
			}
		}

		/*! Execute tasks single threaded */
		inline void Execute_ST_Impl(RenderSystem& render_system, SceneGraph& scene_graph)
		{
			for (decltype(m_num_tasks) i = 0; i < m_num_tasks; ++i)
			{
				// Skip this task if it doesn't need to be executed
				if (!m_should_execute[i])
				{
					continue;
				}

				ExecuteSingleTask(render_system, scene_graph, i);
			}
		}

		/*! Execute a single task */
		inline void ExecuteSingleTask(RenderSystem& rs, SceneGraph& sg, RenderTaskHandle handle)
		{
			auto cmd_list = m_cmd_lists[handle];
			auto render_target = m_render_targets[handle];
			auto rt_properties = m_rt_properties[handle].value();

			switch (m_types[handle])
			{
			case RenderTaskType::DIRECT:
				rs.StartRenderTask(cmd_list, { render_target, rt_properties });
				m_execute_funcs[handle](rs, *this, sg, handle);
				rs.StopRenderTask(cmd_list, { render_target, rt_properties });
				break;
			case RenderTaskType::COMPUTE:
				rs.StartComputeTask(cmd_list, { render_target, rt_properties });
				m_execute_funcs[handle](rs, *this, sg, handle);
				rs.StopComputeTask(cmd_list, { render_target, rt_properties });
				break;
			case RenderTaskType::COPY:
				rs.StartCopyTask(cmd_list, { render_target, rt_properties });
				m_execute_funcs[handle](rs, *this, sg, handle);
				rs.StopCopyTask(cmd_list, { render_target, rt_properties });
				break;
			}
		}

		/*! Get a free unique ID. */
		WISPRENDERER_EXPORT static std::uint64_t GetFreeUID()
		{
			if (!m_free_uids.empty())
			{
				std::uint64_t uid = m_free_uids.top();
				m_free_uids.pop();
				return uid;
			}
			std::uint64_t uid = m_largest_uid;
			m_largest_uid++;
			return uid;
		}

		/*! Get a release a unique ID for reuse. */
		WISPRENDERER_EXPORT static void ReleaseUID(std::uint64_t uid)
		{
			m_free_uids.push(uid);
		}

		RenderSystem* m_render_system;
		/*! The number of tasks we have added. */
		std::uint32_t m_num_tasks;
		/*! The thread pool used for multithreading */
		util::ThreadPool* m_thread_pool;

		/*! Vectors which allow us to itterate over only single threader or only multithreaded tasks. */
		std::vector<RenderTaskHandle> m_multi_threaded_tasks;
		std::vector<RenderTaskHandle> m_single_threaded_tasks;

		/*! Holds the textures that can be written to memory. */
		CPUTextures m_output_cpu_textures;

		/*! Task function pointers. */
		std::vector<setup_func_t> m_setup_funcs;
		std::vector<execute_func_t> m_execute_funcs;
		std::vector<destroy_func_t> m_destroy_funcs;
		/*! Task target and command list. */
		std::vector<CommandList*> m_cmd_lists;
		std::vector<RenderTarget*> m_render_targets;
		/*! Task data and the type information of the original data structure. */
		std::vector<void*> m_data;
		std::vector<std::reference_wrapper<const std::type_info>> m_data_type_info;
		/*! Task settings that can be passed to the frame graph from outside the task. */
		std::vector<std::optional<std::any>> m_settings;
		/*! Defines whether a task should execute or not. */
		std::vector<bool> m_should_execute;
		/*! Used to queue a request to change the should execute value */
		std::queue<std::pair<RenderTaskHandle, bool>> m_should_execute_change_request;
		/*! Descriptions of the tasks. */
#ifndef FG_MAX_PERFORMANCE
		/*! Stored the dependencies of a task. */
		std::vector<std::vector<std::reference_wrapper<const std::type_info>>> m_dependencies;
#endif
		std::vector<RenderTaskType> m_types;
		std::vector<std::optional<RenderTargetProperties>> m_rt_properties;
		std::vector<std::future<void>> m_futures;

		const std::uint64_t m_uid;
		static inline std::uint64_t m_largest_uid = 0;
		static inline std::stack<std::uint64_t> m_free_uids = {};
	};

} /* wr */
