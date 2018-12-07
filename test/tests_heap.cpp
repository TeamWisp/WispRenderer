#include "d3d12/d3d12_structs.hpp"
#include "d3d12/d3d12_functions.hpp"
#include "renderer.hpp"
#include "window.hpp"
#include "d3d12/d3d12_renderer.hpp"

#include <gtest\gtest.h>

TEST(HeapTest, SmallHeapCreation) 
{
	auto render_system = std::make_unique<wr::D3D12RenderSystem>();
	auto window = std::make_unique<wr::Window>(GetModuleHandleA(nullptr), "D3D12 Test App", 1280, 720);

	window->SetKeyCallback([](int key, int action, int mods)
	{
		
	});

	render_system->Init(window.get());

	wr::d3d12::Heap<wr::HeapOptimization::SMALL_BUFFERS>* heap = wr::d3d12::CreateHeap_SBO(
		render_system->m_device,
		70000, wr::ResourceType::BUFFER,
		3);

	EXPECT_EQ(heap->m_heap_size, 131072) << "Heap size is " << heap->m_heap_size << " instead of 131072";
	EXPECT_EQ(heap->m_alignment, 256) << "Heap alignment is " << heap->m_alignment << " instead of 256";
	EXPECT_EQ(heap->m_bitmap.size(), 8) << "Number of memory pages ints is " << heap->m_bitmap.size() << " instead of 8";
	for (int i = 0; i < heap->m_bitmap.size(); ++i) 
	{
		EXPECT_EQ(heap->m_bitmap[i], 0xffffffffffffffff) << "Not all memory pages at int " << i << " are free";
	}
	EXPECT_EQ(heap->m_versioning_count, 3) << "Versioning count is " << heap->m_versioning_count << " instead of 3";
	EXPECT_NE(heap->m_native, nullptr) << "Heap m_native is nullptr";

	D3D12_HEAP_PROPERTIES heap_properties = {};
	D3D12_HEAP_FLAGS heap_flags = {};

	heap->m_native->GetHeapProperties(&heap_properties, &heap_flags);

	EXPECT_EQ(heap_properties.Type, D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD) << 
		"Heap type is " << 
		heap_properties.Type << 
		" instead of D3D12_HEAP_TYPE_UPLOAD (" << 
		D3D12_HEAP_TYPE_UPLOAD << 
		")";

	wr::d3d12::Destroy(heap);

	heap = wr::d3d12::CreateHeap_SBO(
		render_system->m_device,
		500, wr::ResourceType::BUFFER,
		1);

	EXPECT_EQ(heap->m_heap_size, 65536) << "Heap size is " << heap->m_heap_size << " instead of 65536";
	EXPECT_EQ(heap->m_alignment, 256) << "Heap alignment is " << heap->m_alignment << " instead of 256";
	EXPECT_EQ(heap->m_bitmap.size(), 4) << "Number of memory pages ints is " << heap->m_bitmap.size() << " instead of 4";
	for (int i = 0; i < heap->m_bitmap.size(); ++i) 
	{
		EXPECT_EQ(heap->m_bitmap[i], 0xffffffffffffffff) << "Not all memory pages at int " << i << " are free";
	}
	EXPECT_EQ(heap->m_versioning_count, 1) << "Versioning count is " << heap->m_versioning_count << " instead of 1";
	EXPECT_NE(heap->m_native, nullptr) << "Heap m_native is nullptr";

	heap_properties = {};
	heap_flags = {};

	heap->m_native->GetHeapProperties(&heap_properties, &heap_flags);

	EXPECT_EQ(heap_properties.Type, D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD) <<
		"Heap type is " <<
		heap_properties.Type <<
		" instead of D3D12_HEAP_TYPE_UPLOAD (" <<
		D3D12_HEAP_TYPE_UPLOAD <<
		")";

	wr::d3d12::Destroy(heap);

	render_system.reset();
	window.reset();
}

TEST(HeapTest, BigHeapCreation) 
{
	auto render_system = std::make_unique<wr::D3D12RenderSystem>();
	auto window = std::make_unique<wr::Window>(GetModuleHandleA(nullptr), "D3D12 Test App", 1280, 720);

	window->SetKeyCallback([](int key, int action, int mods)
	{

	});

	render_system->Init(window.get());

	wr::d3d12::Heap<wr::HeapOptimization::BIG_BUFFERS>* heap = wr::d3d12::CreateHeap_BBO(
		render_system->m_device,
		170000, wr::ResourceType::BUFFER,
		3);

	EXPECT_EQ(heap->m_heap_size, 196608) << "Heap size is " << heap->m_heap_size << " instead of 196608";
	EXPECT_EQ(heap->m_alignment, 65536) << "Heap alignment is " << heap->m_alignment << " instead of 65536";
	EXPECT_EQ(heap->m_bitmap.size(), 1) << "Number of memory pages ints is " << heap->m_bitmap.size() << " instead of 1";
	for (int i = 0; i < heap->m_bitmap.size(); ++i) 
	{
		EXPECT_EQ(heap->m_bitmap[i], 0xffffffffffffffff) << "Not all memory pages at int " << i << " are free";
	}
	EXPECT_EQ(heap->m_versioning_count, 3) << "Versioning count is " << heap->m_versioning_count << " instead of 3";
	EXPECT_NE(heap->m_native, nullptr) << "Heap m_native is nullptr";

	D3D12_HEAP_PROPERTIES heap_properties = {};
	D3D12_HEAP_FLAGS heap_flags = {};

	heap_properties = heap->m_native->GetDesc().Properties;

	EXPECT_EQ(heap_properties.Type, D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD) <<
		"Heap type is " <<
		heap_properties.Type <<
		" instead of D3D12_HEAP_TYPE_UPLOAD (" <<
		D3D12_HEAP_TYPE_UPLOAD <<
		")";

	wr::d3d12::Destroy(heap);

	heap = wr::d3d12::CreateHeap_BBO(
		render_system->m_device,
		16*1024*1024, wr::ResourceType::BUFFER,
		1);

	EXPECT_EQ(heap->m_heap_size, 16*1024*1024) << "Heap size is " << heap->m_heap_size << " instead of 16777215";
	EXPECT_EQ(heap->m_alignment, 65536) << "Heap alignment is " << heap->m_alignment << " instead of 65536";
	EXPECT_EQ(heap->m_bitmap.size(), 4) << "Number of memory pages ints is " << heap->m_bitmap.size() << " instead of 4";
	for (int i = 0; i < heap->m_bitmap.size(); ++i)
	{
		EXPECT_EQ(heap->m_bitmap[i], 0xffffffffffffffff) << "Not all memory pages at int " << i << " are free";
	}
	EXPECT_EQ(heap->m_versioning_count, 1) << "Versioning count is " << heap->m_versioning_count << " instead of 1";
	EXPECT_NE(heap->m_native, nullptr) << "Heap m_native is nullptr";

	heap_properties = {};
	heap_flags = {};

	heap_properties = heap->m_native->GetDesc().Properties;

	EXPECT_EQ(heap_properties.Type, D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD) <<
		"Heap type is " <<
		heap_properties.Type <<
		" instead of D3D12_HEAP_TYPE_UPLOAD (" <<
		D3D12_HEAP_TYPE_UPLOAD <<
		")";

	wr::d3d12::Destroy(heap);

	render_system.reset();
	window.reset();
}

TEST(HeapTest, BigStaticHeapCreation)
{
	auto render_system = std::make_unique<wr::D3D12RenderSystem>();
	auto window = std::make_unique<wr::Window>(GetModuleHandleA(nullptr), "D3D12 Test App", 1280, 720);

	window->SetKeyCallback([](int key, int action, int mods)
	{

	});

	render_system->Init(window.get());

	wr::d3d12::Heap<wr::HeapOptimization::BIG_STATIC_BUFFERS>* heap = wr::d3d12::CreateHeap_BSBO(
		render_system->m_device,
		170000, wr::ResourceType::BUFFER,
		3);

	EXPECT_EQ(heap->m_heap_size, 196608) << "Heap size is " << heap->m_heap_size << " instead of 196608";
	EXPECT_EQ(heap->m_alignment, 65536) << "Heap alignment is " << heap->m_alignment << " instead of 65536";
	EXPECT_EQ(heap->m_bitmap.size(), 1) << "Number of memory pages ints is " << heap->m_bitmap.size() << " instead of 1";
	for (int i = 0; i < heap->m_bitmap.size(); ++i) 
	{
		EXPECT_EQ(heap->m_bitmap[i], 0xffffffffffffffff) << "Not all memory pages at int " << i << " are free";
	}
	EXPECT_EQ(heap->m_versioning_count, 3) << "Versioning count is " << heap->m_versioning_count << " instead of 3";
	EXPECT_NE(heap->m_native, nullptr) << "Heap m_native is nullptr";

	D3D12_HEAP_PROPERTIES heap_properties = {};
	D3D12_HEAP_FLAGS heap_flags = {};

	heap_properties = heap->m_native->GetDesc().Properties;

	EXPECT_EQ(heap_properties.Type, D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT) <<
		"Heap type is " <<
		heap_properties.Type <<
		" instead of D3D12_HEAP_TYPE_UPLOAD (" <<
		D3D12_HEAP_TYPE_DEFAULT <<
		")";

	wr::d3d12::Destroy(heap);

	heap = wr::d3d12::CreateHeap_BSBO(
		render_system->m_device,
		16 * 1024 * 1024, wr::ResourceType::BUFFER,
		1);

	EXPECT_EQ(heap->m_heap_size, 16 * 1024 * 1024) << "Heap size is " << heap->m_heap_size << " instead of 16777215";
	EXPECT_EQ(heap->m_alignment, 65536) << "Heap alignment is " << heap->m_alignment << " instead of 65536";
	EXPECT_EQ(heap->m_bitmap.size(), 4) << "Number of memory pages ints is " << heap->m_bitmap.size() << " instead of 4";
	for (int i = 0; i < heap->m_bitmap.size(); ++i)
	{
		EXPECT_EQ(heap->m_bitmap[i], 0xffffffffffffffff) << "Not all memory pages at int " << i << " are free";
	}
	EXPECT_EQ(heap->m_versioning_count, 1) << "Versioning count is " << heap->m_versioning_count << " instead of 1";
	EXPECT_NE(heap->m_native, nullptr) << "Heap m_native is nullptr";
	
	heap_properties = {};
	heap_flags = {};

	heap_properties = heap->m_native->GetDesc().Properties;

	EXPECT_EQ(heap_properties.Type, D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT) <<
		"Heap type is " <<
		heap_properties.Type <<
		" instead of D3D12_HEAP_TYPE_UPLOAD (" <<
		D3D12_HEAP_TYPE_DEFAULT <<
		")";

	wr::d3d12::Destroy(heap);

	render_system.reset();
	window.reset();
}

TEST(HeapTest, SmallHeapAllocDealloc) 
{
	auto render_system = std::make_unique<wr::D3D12RenderSystem>();
	auto window = std::make_unique<wr::Window>(GetModuleHandleA(nullptr), "D3D12 Test App", 1280, 720);

	window->SetKeyCallback([](int key, int action, int mods)
	{

	});

	render_system->Init(window.get());

	wr::d3d12::Heap<wr::HeapOptimization::SMALL_BUFFERS>* heap = wr::d3d12::CreateHeap_SBO(
		render_system->m_device,
		70000, wr::ResourceType::BUFFER,
		3);

	wr::d3d12::MapHeap(heap);

	for (int i = 0; i < heap->m_bitmap.size(); ++i) 
	{
		EXPECT_EQ(heap->m_bitmap[i], 0xffffffffffffffff) << "Not all memory pages at int " << i << " are free";
	}

	wr::d3d12::HeapResource* resource = wr::d3d12::AllocConstantBuffer(heap, 60);

	EXPECT_EQ(resource->m_heap_sbo, heap) << "Buffer heap isn't the original heap";

	EXPECT_EQ(resource->m_resource_heap_optimization, wr::HeapOptimization::SMALL_BUFFERS) <<
		"Heap optimization is " <<
		resource->m_resource_heap_optimization <<
		" instead of SMALL_BUFFERS (" <<
		wr::SMALL_BUFFERS <<
		")";

	EXPECT_EQ(resource->m_unaligned_size, 60) << "Resource unaligned size is " << resource->m_unaligned_size << " instead of 60";
	
	for (int i = 0; i < 3; ++i) 
	{
		EXPECT_EQ(resource->m_cpu_addresses->at(i), heap->m_cpu_address + i * 256) << 
			"resource cpu address " << 
			i << 
			" is " << 
			resource->m_cpu_addresses->at( i ) <<
			" instead of " << 
			heap->m_cpu_address + i * 256;
	}

	for (int i = 0; i < 3; ++i) 
	{
		EXPECT_EQ(resource->m_gpu_addresses[i], heap->m_native->GetGPUVirtualAddress() + i * 256) <<
			"resource gpu address " <<
			i <<
			" is " <<
			resource->m_gpu_addresses[i] <<
			" instead of " <<
			heap->m_native->GetGPUVirtualAddress() + i * 256;
	}

	EXPECT_EQ(heap->m_bitmap[0], 0xFFFFFFFFFFFFFFF8) << "Allocator bit pattern is " << heap->m_bitmap[0] << " instead of 0xFFFFFFFFFFFFFFF8";

	wr::d3d12::HeapResource* resource_2 = wr::d3d12::AllocConstantBuffer(heap, 512);

	EXPECT_EQ(resource_2->m_unaligned_size, 512) << "Resource unaligned size is " << resource->m_unaligned_size << " instead of 512";

	for (int i = 0; i < 3; ++i)
	{
		EXPECT_EQ(resource_2->m_cpu_addresses->at( i ), heap->m_cpu_address + i * 512) <<
			"resource cpu address " <<
			i <<
			" is " <<
			resource_2->m_cpu_addresses->at( i ) <<
			" instead of " <<
			heap->m_cpu_address + i * 512;
	}

	for (int i = 0; i < 3; ++i)
	{
		EXPECT_EQ(resource_2->m_gpu_addresses[i], heap->m_native->GetGPUVirtualAddress() + i * 512) <<
			"resource gpu address " <<
			i <<
			" is " <<
			resource_2->m_gpu_addresses[i] <<
			" instead of " <<
			heap->m_native->GetGPUVirtualAddress() + i * 512;
	}

	EXPECT_EQ(heap->m_bitmap[0], 0xFFFFFFFFFFFFFE00) << "Allocator bit pattern is " << heap->m_bitmap[0] << " instead of 0xFFFFFFFFFFFFFE00";

	wr::d3d12::DeallocConstantBuffer(heap, resource);

	EXPECT_EQ(heap->m_bitmap[0], 0xFFFFFFFFFFFFFE07) << "Allocator bit pattern is " << heap->m_bitmap[0] << " instead of 0xFFFFFFFFFFFFFE07";

	resource = wr::d3d12::AllocConstantBuffer(heap, 60);

	EXPECT_EQ(heap->m_bitmap[0], 0xFFFFFFFFFFFFFE00) << "Allocator bit pattern is " << heap->m_bitmap[0] << " instead of 0xFFFFFFFFFFFFFE00";

	wr::d3d12::DeallocConstantBuffer(heap, resource);
	wr::d3d12::DeallocConstantBuffer(heap, resource_2);

	EXPECT_EQ(heap->m_bitmap[0], 0xFFFFFFFFFFFFFFFF) << "Allocator bit pattern is " << heap->m_bitmap[0] << " instead of 0xFFFFFFFFFFFFFFFF";

	wr::d3d12::Destroy(heap);
	render_system.reset();
	window.reset();
}

TEST(HeapTest, BigHeapAllocDealloc)
{
	auto render_system = std::make_unique<wr::D3D12RenderSystem>();
	auto window = std::make_unique<wr::Window>(GetModuleHandleA(nullptr), "D3D12 Test App", 1280, 720);

	window->SetKeyCallback([](int key, int action, int mods)
	{

	});

	render_system->Init(window.get());

	wr::d3d12::Heap<wr::HeapOptimization::BIG_BUFFERS>* heap = wr::d3d12::CreateHeap_BBO(
		render_system->m_device,
		32*1024*1024, wr::ResourceType::BUFFER,
		3);

	wr::d3d12::MapHeap(heap);

	for (int i = 0; i < heap->m_bitmap.size(); ++i) 
	{
		EXPECT_EQ(heap->m_bitmap[i], 0xffffffffffffffff) << "Not all memory pages at int " << i << " are free";
	}

	wr::d3d12::HeapResource* resource = wr::d3d12::AllocConstantBuffer(heap, 15360);

	EXPECT_EQ(resource->m_heap_bbo, heap) << "Buffer heap isn't the original heap";

	EXPECT_EQ(resource->m_resource_heap_optimization, wr::HeapOptimization::BIG_BUFFERS) <<
		"Heap optimization is " <<
		resource->m_resource_heap_optimization <<
		" instead of BIG_BUFFERS (" <<
		wr::BIG_BUFFERS <<
		")";

	EXPECT_EQ(resource->m_unaligned_size, 15360) << "Resource unaligned size is " << resource->m_unaligned_size << " instead of 15360";
	
	EXPECT_EQ(heap->m_bitmap[0], 0xFFFFFFFFFFFFFFF8) << "Allocator bit pattern is " << heap->m_bitmap[0] << " instead of 0xFFFFFFFFFFFFFFF8";

	wr::d3d12::HeapResource* resource_2 = wr::d3d12::AllocConstantBuffer(heap, 131072);

	EXPECT_EQ(resource_2->m_unaligned_size, 131072) << "Resource unaligned size is " << resource_2->m_unaligned_size << " instead of 131072";

	EXPECT_EQ(heap->m_bitmap[0], 0xFFFFFFFFFFFFFE00) << "Allocator bit pattern is " << heap->m_bitmap[0] << " instead of 0xFFFFFFFFFFFFFE00";

	wr::d3d12::DeallocConstantBuffer(heap, resource);

	EXPECT_EQ(heap->m_bitmap[0], 0xFFFFFFFFFFFFFE07) << "Allocator bit pattern is " << heap->m_bitmap[0] << " instead of 0xFFFFFFFFFFFFFE07";

	resource = wr::d3d12::AllocConstantBuffer(heap, 15360);

	EXPECT_EQ(heap->m_bitmap[0], 0xFFFFFFFFFFFFFE00) << "Allocator bit pattern is " << heap->m_bitmap[0] << " instead of 0xFFFFFFFFFFFFFE00";

	wr::d3d12::DeallocConstantBuffer(heap, resource);
	wr::d3d12::DeallocConstantBuffer(heap, resource_2);

	EXPECT_EQ(heap->m_bitmap[0], 0xFFFFFFFFFFFFFFFF) << "Allocator bit pattern is " << heap->m_bitmap[0] << " instead of 0xFFFFFFFFFFFFFFFF";

	wr::d3d12::Destroy(heap);
	render_system.reset();
	window.reset();
}

TEST(HeapTest, BigStaticHeapAllocDealloc) 
{
	auto render_system = std::make_unique<wr::D3D12RenderSystem>();
	auto window = std::make_unique<wr::Window>(GetModuleHandleA(nullptr), "D3D12 Test App", 1280, 720);

	window->SetKeyCallback([](int key, int action, int mods)
	{

	});

	render_system->Init(window.get());

	wr::d3d12::Heap<wr::HeapOptimization::BIG_STATIC_BUFFERS>* heap = wr::d3d12::CreateHeap_BSBO(
		render_system->m_device,
		32 * 1024 * 1024, wr::ResourceType::BUFFER,
		3);

	for (int i = 0; i < heap->m_bitmap.size(); ++i) 
	{
		EXPECT_EQ(heap->m_bitmap[i], 0xffffffffffffffff) << "Not all memory pages at int " << i << " are free";
	}

	wr::d3d12::HeapResource* resource = wr::d3d12::AllocStructuredBuffer(heap, 15360, 1, false);

	EXPECT_EQ(resource->m_stride, 1) << "Stride is " << resource->m_stride << " instead of 1";

	EXPECT_EQ(resource->m_heap_bsbo, heap) << "Buffer heap isn't the original heap";

	EXPECT_EQ(resource->m_resource_heap_optimization, wr::HeapOptimization::BIG_STATIC_BUFFERS) <<
		"Heap optimization is " <<
		resource->m_resource_heap_optimization <<
		" instead of BIG_STATIC_BUFFERS (" <<
		wr::BIG_STATIC_BUFFERS <<
		")";

	EXPECT_EQ(resource->m_unaligned_size, 15360) << "Resource unaligned size is " << resource->m_unaligned_size << " instead of 15360";

	EXPECT_EQ(heap->m_bitmap[0], 0xFFFFFFFFFFFFFFF8) << "Allocator bit pattern is " << heap->m_bitmap[0] << " instead of 0xFFFFFFFFFFFFFFF8";

	wr::d3d12::HeapResource* resource_2 = wr::d3d12::AllocStructuredBuffer(heap, 131072, 32, false);

	EXPECT_EQ(resource_2->m_stride, 32) << "Stride is " << resource_2->m_stride << " instead of 32";

	EXPECT_EQ(resource_2->m_unaligned_size, 131072) << "Resource unaligned size is " << resource_2->m_unaligned_size << " instead of 131072";

	EXPECT_EQ(heap->m_bitmap[0], 0xFFFFFFFFFFFFFE00) << "Allocator bit pattern is " << heap->m_bitmap[0] << " instead of 0xFFFFFFFFFFFFFE00";

	wr::d3d12::DeallocBuffer(heap, resource);

	EXPECT_EQ(heap->m_bitmap[0], 0xFFFFFFFFFFFFFE07) << "Allocator bit pattern is " << heap->m_bitmap[0] << " instead of 0xFFFFFFFFFFFFFE07";

	resource = wr::d3d12::AllocStructuredBuffer(heap, 15360, 1, false);

	EXPECT_EQ(heap->m_bitmap[0], 0xFFFFFFFFFFFFFE00) << "Allocator bit pattern is " << heap->m_bitmap[0] << " instead of 0xFFFFFFFFFFFFFE00";

	wr::d3d12::DeallocBuffer(heap, resource);
	wr::d3d12::DeallocBuffer(heap, resource_2);

	EXPECT_EQ(heap->m_bitmap[0], 0xFFFFFFFFFFFFFFFF) << "Allocator bit pattern is " << heap->m_bitmap[0] << " instead of 0xFFFFFFFFFFFFFFFF";

	wr::d3d12::Destroy(heap);
	render_system.reset();
	window.reset();
}