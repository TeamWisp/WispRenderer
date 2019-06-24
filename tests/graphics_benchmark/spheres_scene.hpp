#pragma once

#include "..//common/scene.hpp"

class SpheresScene : public Scene
{
public:
	SpheresScene();

//	void Update() final;

protected:
	void LoadResources() final;
	//void BuildScene(unsigned int width, unsigned int height) final;

private:
	float Lerp(float v0, float v1, float t);

	// Models
	wr::Model* m_sphere_model;

	// Textures
	wr::TextureHandle m_skybox;
	wr::TextureHandle m_flat_normal;
};