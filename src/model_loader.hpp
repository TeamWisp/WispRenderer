#pragma once
#include <vector>

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

	struct Vector3
	{
		float x;
		float y;
		float z;

		float& operator[](int x)
		{
			if (x == 0)
				return this->x;
			if (x == 1)
				return this->y;
			if (x == 2)
				return this->z;
		};
	};

	struct Vector4
	{
		float x;
		float y;
		float z;
		float w;

		float& operator[](int x)
		{
			if (x == 0)
				return this->x;
			if (x == 1)
				return this->y;
			if (x == 2)
				return this->z;
			if (x == 3)
				return this->w;
		};
	};

	struct Matrix4x4
	{
		Vector4 x;
		Vector4 y;
		Vector4 z;
		Vector4 w;

		Vector4& operator[](int x)
		{
			if (x == 0)
				return this->x;
			if (x == 1)
				return this->y;
			if (x == 2)
				return this->z;
			if (x == 3)
				return this->w;
		};
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
		std::vector<Vector3> m_positions;
		std::vector<Vector3> m_colors;
		std::vector<Vector3> m_normals;
		std::vector<Vector3> m_uvw;
		std::vector<Vector3> m_tangents;
		std::vector<Vector3> m_bitangents;

		std::vector<Vector4> m_bone_weights;

		std::vector<Vector4> m_bone_ids;

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

		Vector3 m_base_color;
		Vector3 m_base_metallic;
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
		virtual ~ModelLoader();

		[[nodiscard]] ModelData* Load(std::string_view model_path);
		[[nodiscard]] ModelData* Load(void* data, std::size_t length, std::string format);

		// Cleans up the allocated memory for the model data. 
		// Call this after you've finished interprenting the data. 
		// Destroying the loader works as well.
		void DeleteModel(ModelData* model);

		std::vector<std::string> SupportedModelFormats();

		bool SupportsModelFormat(std::string model_format);

	protected:
		virtual ModelData* LoadModel(std::string_view model_path) = 0;
		virtual ModelData* LoadModel(void* data, std::size_t length, std::string format) = 0;

		std::vector<ModelData*> m_loaded_models;
		std::vector<std::string> m_supported_model_formats;
	};
} /* wr */