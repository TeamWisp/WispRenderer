#pragma once

//! A Scene Graph Implementation Binder Macro
/*!
  This macro is used to link a initialization and a render function to a specific node type.
*/
#define LINK_NODE_FUNCTION(renderer_type, node_type, update_function) \
decltype(node_type::update_func_impl) node_type::update_func_impl = [](wr::RenderSystem* render_system, Node* node) \
{ \
	static_cast<renderer_type*>(render_system)->update_function(static_cast<node_type*>(node)); \
};

//! World Up
static constexpr float world_up[3] = {0, 1, 0};

//! Used to check whether a method exists in a class.
#define DEFINE_HAS_METHOD(f) \
template<typename, typename T> \
struct HasMethod_##f { \
    static_assert( \
        std::integral_constant<T, false>::value, \
        "Second template parameter needs to be of function type."); \
}; \
 \
template<typename C, typename Ret, typename... Args> \
struct HasMethod_##f<C, Ret(Args...)> { \
private: \
    template<typename T> \
    static constexpr auto Check(T*) \
    -> typename \
        std::is_same< \
            decltype( std::declval<T>().f( std::declval<Args>()... ) ), \
            Ret \
        >::type; \
 \
    template<typename> \
    static constexpr std::false_type Check(...); \
 \
    typedef decltype(Check<C>(0)) type; \
 \
public: \
    static constexpr bool value = type::value; \
};

DEFINE_HAS_METHOD(GetInputLayout)

#define IS_PROPER_VERTEX_CLASS(type) static_assert(HasMethod_GetInputLayout<type, std::vector<D3D12_INPUT_ELEMENT_DESC>()>::value, "Could not locate the required type::GetInputLayout function. If intelisense gives you this error ignore it.");

//! Defines to make linking to sg easier.
#define LINK_SG_RENDER_MESHES(renderer_type, function) \
decltype(wr::SceneGraph::m_render_meshes_func_impl) wr::SceneGraph::m_render_meshes_func_impl = [](wr::RenderSystem* render_system, wr::temp::MeshBatches& nodes, wr::CommandList* cmd_list) \
{ \
	static_cast<renderer_type*>(render_system)->function(nodes, cmd_list); \
};

#define LINK_SG_INIT_MESHES(renderer_type, function) \
decltype(wr::SceneGraph::m_init_meshes_func_impl) wr::SceneGraph::m_init_meshes_func_impl = [](wr::RenderSystem* render_system, std::vector<std::shared_ptr<wr::MeshNode>>& nodes) \
{ \
	static_cast<renderer_type*>(render_system)->function(nodes); \
};
#define LINK_SG_INIT_CAMERAS(renderer_type, function) \
decltype(wr::SceneGraph::m_init_cameras_func_impl) wr::SceneGraph::m_init_cameras_func_impl = [](wr::RenderSystem* render_system, std::vector<std::shared_ptr<wr::CameraNode>>& nodes) \
{ \
	static_cast<renderer_type*>(render_system)->function(nodes); \
};
#define LINK_SG_INIT_LIGHTS(renderer_type, function) \
decltype(wr::SceneGraph::m_init_lights_func_impl) wr::SceneGraph::m_init_lights_func_impl = [](wr::RenderSystem* render_system, std::vector<std::shared_ptr<wr::LightNode>>& nodes, std::vector<Light>& lights) \
{ \
	static_cast<renderer_type*>(render_system)->function(nodes, lights); \
};

#define LINK_SG_UPDATE_MESHES(renderer_type, function) \
decltype(wr::SceneGraph::m_update_meshes_func_impl) wr::SceneGraph::m_update_meshes_func_impl = [](wr::RenderSystem* render_system, std::vector<std::shared_ptr<wr::MeshNode>>& nodes) \
{ \
	static_cast<renderer_type*>(render_system)->function(nodes); \
};
#define LINK_SG_UPDATE_CAMERAS(renderer_type, function) \
decltype(wr::SceneGraph::m_update_cameras_func_impl) wr::SceneGraph::m_update_cameras_func_impl = [](wr::RenderSystem* render_system, std::vector<std::shared_ptr<wr::CameraNode>>& nodes) \
{ \
	static_cast<renderer_type*>(render_system)->function(nodes); \
};
#define LINK_SG_UPDATE_LIGHTS(renderer_type, function) \
decltype(wr::SceneGraph::m_update_lights_func_impl) wr::SceneGraph::m_update_lights_func_impl = [](wr::RenderSystem* render_system, std::vector<std::shared_ptr<wr::LightNode>>& nodes, std::vector<Light>& lights, wr::StructuredBufferHandle* structured_buffer, wr::CommandList* cmd_list) \
{ \
	static_cast<renderer_type*>(render_system)->function(nodes, lights, structured_buffer, cmd_list); \
};