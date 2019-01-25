#include "physics_engine.hpp"

#include "phys_node.hpp"

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
		phys_world->setGravity(btVector3(0, 0, 10));

	}

	btSphereShape* PhysicsEngine::CreateSphereShape(const float radius)
	{
		auto shape = new btSphereShape(radius);
		collision_shapes.push_back(shape);
		return shape;
	}

	std::vector<btConvexTriangleMeshShape*> PhysicsEngine::CreateConvexShape(wr::Model* model)
	{
		std::vector<btConvexTriangleMeshShape*> hulls;

		for (auto& mesh_data : model->m_data->m_meshes)
		{
			btTriangleMesh* tri_m = new btTriangleMesh();
			for (auto i = 0; i < mesh_data->m_indices.size(); i += 3)
			{
				auto pos_a = mesh_data->m_positions[i];
				auto pos_b = mesh_data->m_positions[i+1];
				auto pos_c = mesh_data->m_positions[i+2];

				tri_m->addTriangle(
					btVector3(pos_a.x, pos_a.y, pos_a.z),
					btVector3(pos_b.x, pos_b.y, pos_b.z),
					btVector3(pos_c.x, pos_c.y, pos_c.z)
				);
			}
			btConvexTriangleMeshShape* shape = new btConvexTriangleMeshShape(tri_m);

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
#endif  //

		body->setUserIndex(-1);
		phys_world->addRigidBody(body);
		return body;
	}

	void PhysicsEngine::UpdateSim(float delta, wr::SceneGraph& sg)
	{
		phys_world->stepSimulation(delta);

		for (auto& n : sg.GetMeshNodes())
		{
			if (auto& node = std::dynamic_pointer_cast<PhysicsMeshNode>(n))
			{
				if (!node->m_rigid_bodys.has_value() && node->m_rigid_body)
				{
					auto world_position = node->m_rigid_body->getWorldTransform().getOrigin();
					node->m_position = util::DXV3toBV3(world_position);
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