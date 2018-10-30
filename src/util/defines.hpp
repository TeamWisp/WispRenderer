#pragma once

//! A Scene Graph Implementation Binder Macro
/*!
  This macro is used to link a initialization and a render function to a specific node type.
*/
#define LINK_NODE_FUNCTION(renderer_type, node_type, init_function, render_function) \
decltype(node_type::init_func_impl) node_type::init_func_impl = [](wr::RenderSystem* render_system, Node* node) \
{ \
	static_cast<renderer_type*>(render_system)->init_function(static_cast<node_type*>(node)); \
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
static std::function<void(RenderSystem*, Node*)> render_func_impl; \
node_type() { Init = init_func_impl; Render = render_func_impl; }