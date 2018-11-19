#pragma once

//! A Scene Graph Implementation Binder Macro
/*!
  This macro is used to link a initialization and a render function to a specific node type.
*/
#define LINK_NODE_FUNCTION(renderer_type, node_type, init_function, update_function, render_function) \
decltype(node_type::init_func_impl) node_type::init_func_impl = [](wr::RenderSystem* render_system, Node* node) \
{ \
	static_cast<renderer_type*>(render_system)->init_function(static_cast<node_type*>(node)); \
}; \
decltype(node_type::update_func_impl) node_type::update_func_impl = [](wr::RenderSystem* render_system, Node* node) \
{ \
	static_cast<renderer_type*>(render_system)->update_function(static_cast<node_type*>(node)); \
}; \
decltype(node_type::render_func_impl) node_type::render_func_impl = [](wr::RenderSystem* render_system, Node* node) \
{ \
static_cast<renderer_type*>(render_system)->render_function(static_cast<node_type*>(node)); \
};

//! Declare Subnode Properties
/*!
  This macro is used to declare the properties and constructor nessesary to be able to
  link functions to it using the `LINK_NODE_FUNCTION` macro.
*/
#define DECL_SUBNODE(node_type) \
static std::function<void(RenderSystem*, Node*)> init_func_impl; \
static std::function<void(RenderSystem*, Node*)> update_func_impl; \
static std::function<void(RenderSystem*, Node*)> render_func_impl; \
node_type() : Node() { Init = init_func_impl; Update = update_func_impl; Render = render_func_impl; }

#define SUBMODE_CONSTRUCTOR Init = init_func_impl; Update = update_func_impl; Render = render_func_impl;

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