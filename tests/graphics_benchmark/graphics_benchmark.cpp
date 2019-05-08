#include <memory>
#include <algorithm>
#include <thread>
#include <filesystem>

#include "wisp.hpp"
#include "d3d12/d3d12_renderer.hpp"

#include "spheres_scene.hpp"
#include "frame_graphs.hpp"

static const unsigned int default_window_width = 1290;
static const unsigned int default_window_height = 720;
static const unsigned int frames_till_capture = 3;
static const std::string output_dir = "benchmark_images/";
static int benchmark_number = 0;

inline void ReplaceAll(std::string& str, std::string const & original_delimiter, std::string const & new_delimiter)
{
	std::string::size_type n = 0;
	while ((n = str.find(original_delimiter, n)) != std::string::npos)
	{
		str.replace(n, original_delimiter.size(), new_delimiter);
		n += new_delimiter.size();
	}
}

template<typename S, typename O>
void PerformBenchmark(FGType fg_type, unsigned int width = default_window_width, unsigned int height = default_window_height, unsigned int output_render_target_index = 0)
{
	std::string scene_name = std::string(typeid(S).name());
	scene_name.erase(0, 6); // Remove "class " from the name.
	std::transform(scene_name.begin(), scene_name.end(), scene_name.begin(), ::tolower);

	std::string rt_name = std::string(typeid(O).name()) + "_" + std::to_string(output_render_target_index);
	rt_name.erase(0, 6); // Remove "class " from the name.
	std::transform(rt_name.begin(), rt_name.end(), rt_name.begin(), ::tolower);
	ReplaceAll(rt_name, "::", "-");

	std::string fg_name = GetFrameGraphName(fg_type);
	std::transform(fg_name.begin(), fg_name.end(), fg_name.begin(), ::tolower);
	ReplaceAll(fg_name, " ", "_");

	LOGW("Starting Benchmark \"{}\" for scene \"{}\" with fg \"{}\" to output \"{}\"", benchmark_number, scene_name, fg_name, rt_name);

	auto scene = std::make_unique<S>();

	// Initialize
	auto render_system = std::make_unique<wr::D3D12RenderSystem>();
	std::string window_title = "Benchmark (Scene: " + scene_name + " Fg:" + fg_name + " Rt:" + rt_name + ")";
	auto window = std::make_unique<wr::Window>(GetModuleHandleA(nullptr), window_title, width, height, false);

	wr::ModelLoader* assimp_model_loader = new wr::AssimpModelLoader();

	render_system->Init(window.get());
	scene->Init(render_system.get(), width, height);

	auto frame_graph = CreateFrameGraph(fg_type, *render_system);

	// Render
	unsigned int frame = 0;
	while (window->IsRunning())
	{
		window->PollEvents();

		scene->Update();
		render_system->Render(scene->GetSceneGraph(), *frame_graph);

		// Capture screenshot
		if (frame == frames_till_capture)
		{
			Sleep(300);
			std::string path = output_dir + "wisp_img_" + std::to_string(benchmark_number) + "_" + scene_name + "_" + fg_name + "_" + rt_name + ".tga";
			frame_graph->SaveTaskToDisc<O>(path, output_render_target_index);
			LOGW("Saving output to: {}", output_dir);
		}
		// Quit after capture.
		else if (frame == frames_till_capture + 1)
		{
			window->Stop();
		}

		frame++;
	}

	//window->StartRenderLoop();

	// Shutdown
	render_system->WaitForAllPreviousWork();

	delete assimp_model_loader;
	scene.reset();
	frame_graph.reset();
	render_system.reset();

	benchmark_number++;
}

int GraphicsBenchmarkEntry()
{
	// Create output directory if nessessary.
	if (!std::filesystem::exists(output_dir))
	{
		std::filesystem::create_directory(output_dir);
		LOGW("Created output dir {}", output_dir);
	}

	LOGW("Starting Benchmarks");

	PerformBenchmark<SpheresScene, wr::PostProcessingData>(FGType::PBR_BASIC, 1000, 1000);
	PerformBenchmark<SpheresScene, wr::PostProcessingData>(FGType::PBR_RT_REF_SHADOWS, 1000, 1000);

	LOGW("Benchmarks Finished")

	return 0;
}

WISP_ENTRY(GraphicsBenchmarkEntry)