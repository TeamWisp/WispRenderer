#pragma once

#include <vector>
#include <string>
#include <type_traits>
#include <stack>
#include <deque>
#include <future>

#include "../util/thread_pool.hpp"
#include "../util/delegate.hpp"
#include "../renderer.hpp"
#include "../platform_independend_structs.hpp"
#include "../settings.hpp"
#include "../d3d12/d3d12_settings.hpp"
#include "../structs.hpp"

#define FG_MAX_PERFORMANCE

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
		RenderTaskType m_type;
		/*! The function pointers for the task.*/
		setup_func_t m_setup_func;
		execute_func_t m_execute_func;
		destroy_func_t m_destroy_func;

		/*! The properties for the render target this task renders to. If this is `std::nullopt` no render target will be created. */
		std::optional<RenderTargetProperties> m_properties;

		bool m_allow_multithreading;
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
		using setup_func_t = RenderTaskDesc::setup_func_t;
		using execute_func_t = RenderTaskDesc::execute_func_t;
		using destroy_func_t = RenderTaskDesc::destroy_func_t;
	public:

		//! Constructor.
		/*!
			This constructor is able to reserve space for render tasks.
			This works by calling `std::vector::reserve`.
			\param num_reserved_tasks Amount of tasks we should reserve space for.
		*/
		FrameGraph(std::size_t num_reserved_tasks = 1) : m_num_tasks(0), m_thread_pool(new util::ThreadPool(settings::num_frame_graph_threads)), m_uid(GetFreeUID())
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
			reserve(m_names);
#endif
			reserve(m_types);
			reserve(m_rt_properties);
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
		FrameGraph(FrameGraph&&) = delete;

		FrameGraph& operator=(const FrameGraph&) = delete;
		FrameGraph& operator=(FrameGraph&&) = delete;

		//! Setup the render tasks
		/*!
			Calls all setup function pointers and obtains the required render targets and command lists.
			It is recommended to try to avoid calling this during runtime because it can cause stalls if the setup functions are expensive.
			\param render_system The render system we want to use for rendering.
		*/
		inline void Setup(RenderSystem& render_system)
		{
			// Resize these vectors since we know the end size already.
			m_cmd_lists.resize(m_num_tasks);
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

			if constexpr (settings::use_multithreading)
			{
				Execute_MT_Impl(render_system, scene_graph);
			}
			else
			{
				Execute_ST_Impl(render_system, scene_graph);
			}
		}

		inline void Resize(RenderSystem & render_system, std::uint32_t width, std::uint32_t height)
		{
			for (decltype(m_num_tasks) i = 0; i < m_num_tasks; ++i)
			{
				WaitForCompletion(i);

				m_destroy_funcs[i](*this, i, true);

				if (!m_rt_properties[i].value().m_is_render_window)
				{		
					render_system.ResizeRenderTarget(&m_render_targets[i], 
						width * m_rt_properties[i].value().m_resolution_scale.Get(), 
						height * m_rt_properties[i].value().m_resolution_scale.Get());
				}

				m_setup_funcs[i](render_system, *this, i, true);
			}
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
#ifndef FG_MAX_PERFORMANCE
			m_names.clear();
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

		/*! Get the data of a task. (Modifyable) */
		/*!
			The template variable is used to specify the type of the data structure.
			\param handle The handle to the render task. (Given by the `Setup`, `Execute` and `Destroy` functions)
		*/
		template<typename T = void*>
		inline auto & GetData(RenderTaskHandle handle) const
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
		inline auto const & GetPredecessorData()
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

			LOGC("Failed to find predecessor data! Please check your task order.");
			return *static_cast<T*>(nullptr);
		}

		/*! Get the render target of a previously ran task. (Constant) */
		/*!
			This function loops over all tasks and checks whether it has the same type information as the template variable.
			If no task is found with the type specified a nullptr will be returned and a error message send to the logging system.
			\param handle The handle to the render task. (Given by the `Setup`, `Execute` and `Destroy` functions)
		*/
		template<typename T>
		inline RenderTarget* GetPredecessorRenderTarget()
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
		inline auto GetCommandList(RenderTaskHandle handle) const
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
		inline wr::CommandList* GetPredecessorCommandList()
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
		std::vector<T*> GetAllCommandLists()
		{
			std::vector<T*> retval(m_num_tasks);

			// TODO: Just return the fucking vector as const ref.
			for (decltype(m_num_tasks) i = 0; i < m_num_tasks; i++)
			{
				WaitForCompletion(i);
				retval[i] = static_cast<T*>(m_cmd_lists[i]);
			}

			return retval;
		}

		/*! Get the render target of a task. */
		/*!
			The template variable allows you to cast the render target to a "non platform independent" different type. For example a `D3D12RenderTarget`.
			\param handle The handle to the render task. (Given by the `Setup`, `Execute` and `Destroy` functions)
		*/
		template<typename T = RenderTarget>
		inline auto GetRenderTarget(RenderTaskHandle handle) const
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
			for (decltype(m_num_tasks) i = 0; i < m_num_tasks; ++i)
			{
				if (m_data_type_info[i].get() == typeid(T))
				{
					return true;
				}
			}
			return false;
		}

		/*! Add a task to the Frame Graph. */
		/*!
			This creates a new render task based on a description.
			\param desc A description of the render task.
		*/
		template<typename T>
		inline void AddTask(RenderTaskDesc& desc)
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
			m_names.emplace_back(desc.m_name);
#endif
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
		const std::uint64_t GetUID() const
		{
			return m_uid;
		};

		/*! Return the cpu texture. */
		const CPUTextures& GetOutputTexture()
		{
			return m_output_cpu_textures;
		}

		/*! Set the cpu texture's data. */
		void SetOutputTexture(const CPUTexture& output_texture, CPUTextureType type)
		{
			switch (type)
			{
			case wr::CPUTextureType::PIXEL_DATA:
				if (m_output_cpu_textures.pixel_data != std::nullopt)
					LOGW("Warning: CPU texture pixel data is written to more than once a frame!");

				// Save the pixel data
				m_output_cpu_textures.pixel_data = output_texture;
				break;

			case wr::CPUTextureType::DEPTH_DATA:
				if (m_output_cpu_textures.depth_data != std::nullopt)
					LOGW("Warning: CPU texture depth data is written to more than once a frame!");

				// Save the depth data
				m_output_cpu_textures.depth_data = output_texture;
				break;

			default:
				// Should never happen
				LOGC("Invalid CPU texture type supplied!");
				break;
			}
		}

		/*! Resets the CPU texture data for this frame. */
		void ResetOutputTexture()
		{
			// Frame has been rendered, allow a task to write to the CPU texture in the next frame
			m_output_cpu_textures.pixel_data = std::nullopt;
			m_output_cpu_textures.depth_data = std::nullopt;
		}

	private:

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
				m_futures[handle] = m_thread_pool->Enqueue([this, handle, &render_system, &scene_graph]
				{
					ExecuteSingleTask(render_system, scene_graph, handle);
				});
			}

			// Singlethreading behaviour
			for (const auto handle : m_single_threaded_tasks)
			{
				ExecuteSingleTask(render_system, scene_graph, handle);
			}
		}

		/*! Execute tasks single threaded */
		inline void Execute_ST_Impl(RenderSystem& render_system, SceneGraph& scene_graph)
		{
			for (decltype(m_num_tasks) i = 0; i < m_num_tasks; ++i)
			{
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
		static std::uint64_t GetFreeUID()
		{
			if (m_free_uids.size() > 0)
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
		static void ReleaseUID(std::uint64_t uid)
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
		/*! Descriptions of the tasks. */
#ifndef FG_MAX_PERFORMANCE
		std::vector<const char*> m_names;
#endif
		std::vector<RenderTaskType> m_types;
		std::vector<std::optional<RenderTargetProperties>> m_rt_properties;
		std::vector<std::future<void>> m_futures;

		const std::uint64_t m_uid;
		static std::uint64_t m_largest_uid;
		static std::stack<std::uint64_t> m_free_uids;
	};

} /* wr */