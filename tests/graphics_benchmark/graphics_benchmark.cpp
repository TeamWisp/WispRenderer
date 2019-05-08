#include <memory>
#include <algorithm>
#include <thread>
#include <chrono>

#include "wisp.hpp"
#include "d3d12/d3d12_renderer.hpp"

#include "spheres_scene.hpp"
#include "frame_graphs.hpp"

static const unsigned int window_width = 900;
static const unsigned int window_height = 900;
static const unsigned int frames_till_capture = 2;

template<typename S>
void PerformBenchmark(FGType fg_type)
{
	auto scene = std::make_unique<S>();

	// Initialize
	auto render_system = std::make_unique<wr::D3D12RenderSystem>();
	std::string window_title = "Benchmark (" + std::string(typeid(S).name()) + " FG:{" + std::to_string(static_cast<std::uint32_t>(fg_type)) + "})";
	auto window = std::make_unique<wr::Window>(GetModuleHandleA(nullptr), window_title, window_width, window_height);

	wr::ModelLoader* assimp_model_loader = new wr::AssimpModelLoader();

	render_system->Init(window.get());
	scene->Init(render_system.get(), window_width, window_height);

	auto frame_graph = CreateFrameGraph(fg_type, *render_system);

	// Render
	unsigned int frame = 0;
	window->SetRenderLoop([&]() {
		scene->Update();
		render_system->Render(scene->GetSceneGraph(), *frame_graph);

		// Capture screenshot
		if (frame == frames_till_capture)
		{
		}
		// Quit after capture.
		else if (frame == frames_till_capture + 1)
		{
			window->Stop();
		}

		frame++;
	});

	window->StartRenderLoop();

	// Shutdown
	render_system->WaitForAllPreviousWork();

	delete assimp_model_loader;
	scene.reset();
	frame_graph.reset();
	render_system.reset();
}

int GraphicsBenchmarkEntry()
{
	PerformBenchmark<SpheresScene>(FGType::PBR);
	PerformBenchmark<SpheresScene>(FGType::PBR);
	PerformBenchmark<SpheresScene>(FGType::PBR);

	return 0;
}

WISP_ENTRY(GraphicsBenchmarkEntry)