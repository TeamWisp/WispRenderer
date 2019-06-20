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

#include "wisp.hpp"

#include <btBulletDynamicsCommon.h>
#include <LinearMath/btVector3.h>
#include <LinearMath/btAlignedObjectArray.h>
#include <btBulletDynamicsCommon.h>
#include <scene_graph/scene_graph.hpp>

#define USE_MOTIONSTATE 1

namespace wr
{
	struct Model;
}

namespace phys
{
	namespace util
	{
		inline btVector3 DXV3toBV3(DirectX::XMVECTOR v)
		{
			return btVector3(v.m128_f32[0], v.m128_f32[1], v.m128_f32[2]);
		}
		inline DirectX::XMVECTOR BV3toDXV3(btVector3 v)
		{
			return { v.x(), v.y(), v.z() };
		}
	}

	struct PhysicsEngine
	{
		btAlignedObjectArray<btCollisionShape*> collision_shapes;
		btBroadphaseInterface* broadphase;
		btCollisionDispatcher* coll_dispatcher;
		btConstraintSolver* constraint_solver;
		btDefaultCollisionConfiguration* collision_config;
		btDiscreteDynamicsWorld* phys_world;

		void CreatePhysicsWorld();

		btBoxShape* CreateBoxShape(const btVector3& halfExtents);
		btSphereShape* CreateSphereShape(const float radius);
		btCapsuleShape* CreateCapsuleShape(const float width, const float height);
		std::vector<btConvexHullShape*> CreateConvexShape(wr::ModelData* model);

		btRigidBody* CreateRigidBody(float mass, const btTransform& startTransform, btCollisionShape* shape);

		void UpdateSim(float delta, wr::SceneGraph& sg);

		void Destroy();
	};

} /* phys*/