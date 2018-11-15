#pragma once

#include <string_view>
#include <vector>

namespace wr
{
	struct TextureHandle {};
	struct ModelHandle {};
	struct MaterialHandle {};
	struct ConstantBufferHandle {};

	enum class TextureType
	{
		PNG,
		DDS,
		HDR
	};

	enum class ModelType
	{
		FBX
	};

	class MaterialPool
	{
	public:
		explicit MaterialPool(std::size_t size_in_mb);
		virtual ~MaterialPool() = default;

		MaterialPool(MaterialPool const &) = delete;
		MaterialPool& operator=(MaterialPool const &) = delete;
		MaterialPool(MaterialPool&&) = delete;
		MaterialPool& operator=(MaterialPool&&) = delete;

		MaterialHandle Load(std::string_view path, TextureType type);

		virtual void Evict() = 0;
		virtual void MakeResident() = 0;

	protected:
		virtual MaterialHandle LoadMaterial(std::string_view path) = 0;
		virtual TextureHandle LoadPNG(std::string_view path) = 0;
		virtual TextureHandle LoadDDS(std::string_view path) = 0;
		virtual TextureHandle LoadHDR(std::string_view path) = 0;

		std::size_t m_size_in_mb;
	};

	class ModelPool
	{
	public:
		explicit ModelPool(std::size_t size_in_mb);
		virtual ~ModelPool() = default;

		ModelPool(ModelPool const &) = delete;
		ModelPool& operator=(ModelPool const &) = delete;
		ModelPool(ModelPool&&) = delete;
		ModelPool& operator=(ModelPool&&) = delete;

		ModelHandle Load(std::string_view path, ModelType type);
		std::pair<ModelHandle, std::vector<MaterialHandle>> LoadWithMaterials(MaterialPool* material_pool, std::string_view path, ModelType type);
		
		virtual void Evict() = 0;
		virtual void MakeResident() = 0;
	
	protected:
		virtual ModelHandle LoadFBX(std::string_view path, ModelType type) = 0;

		std::size_t m_size_in_mb;
	};

}
