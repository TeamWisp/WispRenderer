#pragma once

#include <btBulletDynamicsCommon.h>
#include <LinearMath/btVector3.h>
#include <LinearMath/btAlignedObjectArray.h>
#include <btBulletDynamicsCommon.h>

#include "scene_graph/mesh_node.hpp"

namespace phys
{
	struct PhysicsEngine;
}

struct PhysicsMeshNode : public wr::MeshNode
{
	btCollisionShape* m_shape = nullptr;
	btRigidBody* m_rigid_body = nullptr;

	std::optional<std::vector<btCollisionShape*>> m_shapes;
	std::optional<std::vector<btRigidBody*>> m_rigid_bodies;

	float m_mass = 0;

	PhysicsMeshNode(wr::Model* model) : MeshNode(model) { }

	void SetMass(float mass);
	void SetRestitution(float value);

	void SetupSimpleBoxColl(phys::PhysicsEngine& phys_engine, DirectX::XMVECTOR scale);
	void SetupSimpleSphereColl(phys::PhysicsEngine& phys_engine, float radius);
	void SetupConvex(phys::PhysicsEngine& phys_engine, wr::ModelData* model);

	void SetPosition(DirectX::XMVECTOR position) override;
	void SetRotation(DirectX::XMVECTOR roll_pitch_yaw) override;
	void SetScale(DirectX::XMVECTOR scale) override;
};