#pragma once
#include <vector>
#include <DirectXMath.h>

namespace wr
{
	struct ModelMeshData;
	struct ModelMaterialData;
	struct ModelData;
	struct ModelSkeletonData;
	struct ModelBoneData;
	struct ModelAnimationData;

	struct EmbeddedTexture;

	enum class TextureLocation
	{
		EXTERNAL = 0,
		EMBEDDED,
		NON_EXISTENT
	};

	struct ModelData
	{
		std::vector<ModelMeshData*> m_meshes;
		std::vector<ModelMaterialData*> m_materials;
		std::vector<EmbeddedTexture*> m_embedded_textures;
		ModelSkeletonData* m_skeleton_data;
	};

	struct ModelMeshData
	{
		std::vector<DirectX::XMFLOAT3> m_positions;
		std::vector<DirectX::XMFLOAT3> m_colors;
		std::vector<DirectX::XMFLOAT3> m_normals;
		std::vector<DirectX::XMFLOAT3> m_uvw;
		std::vector<DirectX::XMFLOAT3> m_tangents;
		std::vector<DirectX::XMFLOAT3> m_bitangents;

		std::vector<DirectX::XMFLOAT4> m_bone_weights;

		std::vector<DirectX::XMFLOAT4> m_bone_ids;

		std::vector<std::uint32_t> m_indices;

		int m_material_id;
	};

	struct ModelMaterialData 
	{
		std::string m_albedo_texture;
		std::size_t m_albedo_embedded_texture;		
		TextureLocation m_albedo_texture_location;

		std::string m_metallic_texture;
		std::size_t m_metallic_embedded_texture;
		TextureLocation m_metallic_texture_location;

		std::string m_roughness_texture;
		std::size_t m_roughness_embedded_texture;
		TextureLocation m_roughness_texture_location;

		std::string m_ambient_occlusion_texture;
		std::size_t m_ambient_occlusion_embedded_texture;
		TextureLocation m_ambient_occlusion_texture_location;

		std::string m_normal_map_texture;
		std::size_t m_normal_map_embedded_texture;
		TextureLocation m_normal_map_texture_location;

		DirectX::XMFLOAT3 m_base_color;
		DirectX::XMFLOAT3 m_base_metallic;
		float m_base_roughness;
		float m_base_transparency;
		
		bool m_two_sided;
	};

	struct ModelBoneData
	{

	};

	struct ModelAnimationData
	{

	};

	struct ModelSkeletonData 
	{
		std::vector<ModelBoneData*> m_bones;
		std::vector<ModelAnimationData*> m_animations;
	};

	struct EmbeddedTexture 
	{
		std::vector<char> m_data;

		std::uint32_t m_width;
		std::uint32_t m_height;

		bool m_compressed;

		std::string format;
	};

	class ModelLoader
	{
	public:
		ModelLoader();
		virtual ~ModelLoader();

		[[nodiscard]] ModelData* Load(std::string_view model_path);
		[[nodiscard]] ModelData* Load(void* data, std::size_t length, std::string format);

		// Cleans up the allocated memory for the model data. 
		// Call this after you've finished interprenting the data. 
		// Destroying the loader works as well.
		void DeleteModel(ModelData* model);

		std::vector<std::string> SupportedModelFormats();

		bool SupportsModelFormat(const std::string_view& model_format);

		static ModelLoader* FindFittingModelLoader(const std::string_view& model_format);

		static std::vector<ModelLoader*> m_registered_model_loaders;

	protected:
		virtual ModelData* LoadModel(std::string_view model_path) = 0;
		virtual ModelData* LoadModel(void* data, std::size_t length, std::string format) = 0;

		std::vector<ModelData*> m_loaded_models;
		std::vector<std::string> m_supported_model_formats;
	};
} /* wr */