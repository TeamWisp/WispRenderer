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
#include "physics_engine.hpp"

#include "physics_node.hpp"
#include "debug_camera.hpp"
#include "model_pool.hpp"

namespace phys
{
	void PhysicsEngine::CreatePhysicsWorld()
	{
		///collision configuration contains default setup for memory, collision setup
		collision_config = new btDefaultCollisionConfiguration();
		//m_collisionConfiguration->setConvexConvexMultipointIterations();

		///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
		coll_dispatcher = new btCollisionDispatcher(collision_config);

		broadphase = new btDbvtBroadphase();

		///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
		constraint_solver = new btSequentialImpulseConstraintSolver();

		phys_world = new btDiscreteDynamicsWorld(coll_dispatcher, broadphase, constraint_solver, collision_config);
		phys_world->setGravity(btVector3(0, -9.8, 0));

	}

	btSphereShape* PhysicsEngine::CreateSphereShape(const float radius)
	{
		auto shape = new btSphereShape(radius);
		collision_shapes.push_back(shape);
		return shape;
	}

	btCapsuleShape* PhysicsEngine::CreateCapsuleShape(const float width, const float height)
	{
		auto shape = new btCapsuleShape(width, height);
		collision_shapes.push_back(shape);
		return shape;
	}

	std::vector<btConvexHullShape*> PhysicsEngine::CreateConvexShape(wr::ModelData* model)
	{
		std::vector<btConvexHullShape*> hulls;

		for (auto& mesh_data : model->m_meshes)
		{
			btConvexHullShape* shape = new btConvexHullShape();


			for (auto& idx : mesh_data->m_indices)
			{

				auto pos = mesh_data->m_positions[idx];
				shape->addPoint(btVector3(pos.x, pos.y, pos.z), false);
			}
			shape->recalcLocalAabb();

			collision_shapes.push_back(shape);
			hulls.push_back(shape);
		}

		return hulls;
	}

	std::vector<btBvhTriangleMeshShape*> PhysicsEngine::CreateTriangleMeshShape(wr::ModelData* model)
	{
		std::vector<btBvhTriangleMeshShape*> hulls;

		for (auto& mesh_data : model->m_meshes)
		{
			btTriangleIndexVertexArray* va = new btTriangleIndexVertexArray(mesh_data->m_indices.size() / 3,
				reinterpret_cast<int*>(mesh_data->m_indices.data()),
				3 * sizeof(std::uint32_t),
				mesh_data->m_positions.size(), reinterpret_cast<btScalar*>(mesh_data->m_positions.data()), sizeof(DirectX::XMFLOAT3));

			btBvhTriangleMeshShape* shape = new btBvhTriangleMeshShape(va, true);
			collision_shapes.push_back(shape);
			hulls.push_back(shape);
		}

		return hulls;
	}

	btBoxShape* PhysicsEngine::CreateBoxShape(const btVector3& halfExtents)
	{
		auto shape = new btBoxShape(halfExtents);
		collision_shapes.push_back(shape);
		return shape;
	}

	btRigidBody* PhysicsEngine::CreateRigidBody(float mass, const btTransform& startTransform, btCollisionShape* shape)
	{
		btAssert((!shape || shape->getShapeType() != INVALID_SHAPE_PROXYTYPE));

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool is_dynamic = (mass != 0.f);

		btVector3 local_inertia(0, 0, 0);
		if (is_dynamic)
			shape->calculateLocalInertia(mass, local_inertia);

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects

#ifdef USE_MOTIONSTATE
		btDefaultMotionState* motion_state = new btDefaultMotionState(startTransform);

		btRigidBody::btRigidBodyConstructionInfo cInfo(mass, motion_state, shape, local_inertia);
		btRigidBody* body = new btRigidBody(cInfo);
		//body->setContactProcessingThreshold(m_defaultContactProcessingThreshold);

#else
		btRigidBody* body = new btRigidBody(mass, 0, shape, localInertia);
		body->setWorldTransform(startTransform);
#endif 

		body->setUserIndex(-1);
		phys_world->addRigidBody(body);
		return body;
	}

	void PhysicsEngine::UpdateSim(float delta, wr::SceneGraph& sg)
	{
		phys_world->stepSimulation(delta);

		for (auto& n : sg.GetMeshNodes())
		{
			if (auto & node = std::dynamic_pointer_cast<PhysicsMeshNode>(n))
			{
				if (!node->m_rigid_bodies.has_value() && node->m_rigid_body)
				{
					auto world_position = node->m_rigid_body->getWorldTransform().getOrigin();
					node->m_position = util::BV3toDXV3(world_position);
					node->SignalTransformChange();
				}
			}
		}
	}

	PhysicsEngine::~PhysicsEngine()
	{
		delete phys_world;
		delete broadphase;
		delete coll_dispatcher;
		delete constraint_solver;
		delete collision_config;
	}

} /* phys*/