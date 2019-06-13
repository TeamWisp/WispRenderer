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
				if (!node->m_rigid_bodys.has_value() && node->m_rigid_body)
				{
					auto world_position = node->m_rigid_body->getWorldTransform().getOrigin();
					node->m_position = util::BV3toDXV3(world_position);
					node->SignalTransformChange();
				}
			}
		}
	}

	void PhysicsEngine::Destroy()
	{

		delete broadphase;
		delete coll_dispatcher;
		delete constraint_solver;
		delete collision_config;
		delete phys_world;
	}

} /* phys*/