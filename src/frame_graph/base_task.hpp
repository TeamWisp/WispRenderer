#pragma once

#include <functional>
#include <string>

namespace wr
{
	class RenderSystem;
	class SceneGraph;
} /* wr */

namespace wr::fg
{

	class BaseTask
	{
		friend class FrameGraph;
	public:
		BaseTask(const std::type_info& data_type_info, FrameGraph* frame_graph, std::string const & name)
			: data_type_info(data_type_info), frame_graph(frame_graph), name(name)
		{
		}

		void SetFrameGraph(FrameGraph* frame_graph)
		{
			this->frame_graph = frame_graph;
		}

		virtual ~BaseTask() = default;

		BaseTask(const BaseTask&) = delete;
		BaseTask(BaseTask&&) = default;
		BaseTask& operator=(const BaseTask&) = delete;
		BaseTask& operator=(BaseTask&&) = default;

		/*
		template<typename T>
		void Create(T** out_resource, std::string const & name, void* data)
		{
			frame_graph->resources.emplace_back(std::make_unique<T>(name, this, data));
			//const auto* resource = frame_graph->resources.back().get();

			//(*out_resource) = static_cast<T*>(resource);
		}

		template<typename T>
		T* Read(T* resource)
		{

		}

		template<typename T>
		T* Write(T* resource)
		{

		}
		*/

		virtual void Setup(wr::RenderSystem&) = 0;
		virtual void Execute(wr::RenderSystem&, SceneGraph&) = 0; // TODO This could be const.

	protected:
		FrameGraph* frame_graph;

		std::string name;
		bool cull_immune;
		//std::size_t ref_count;

		//void* data;
		const std::type_info& data_type_info;
	};

} /* wr::fg */