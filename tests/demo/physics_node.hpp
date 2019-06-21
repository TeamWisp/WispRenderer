/*!
 * Copyright 2019 Breda University of Applied Sciences and Team Wisp (Viktor Zoutman, Emilio Laiso, Jens Hagen, Meine Zeinstra, Tahar Meijs, Koen Buitenhuis, Niels Brunekreef, Darius Bouma, Florian Schut)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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

	phys::PhysicsEngine& m_phys_engine;

	float m_mass = 0;

	PhysicsMeshNode(phys::PhysicsEngine* phys_engine, wr::Model* model) : MeshNode(model), m_phys_engine(*phys_engine) { }
	~PhysicsMeshNode();

	void SetMass(float mass);
	void SetRestitution(float value);

	void SetupSimpleBoxColl(phys::PhysicsEngine& phys_engine, DirectX::XMVECTOR scale);
	void SetupSimpleSphereColl(phys::PhysicsEngine& phys_engine, float radius);
	void SetupConvex(phys::PhysicsEngine& phys_engine, wr::ModelData* model);
	void SetupTriangleMesh(phys::PhysicsEngine& phys_engine, wr::ModelData* model);

	void SetPosition(DirectX::XMVECTOR position) override;
	void SetRotation(DirectX::XMVECTOR roll_pitch_yaw) override;
	void SetScale(DirectX::XMVECTOR scale) override;
};