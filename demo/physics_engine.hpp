#pragma once

#include "wisp.hpp"

#include <btBulletDynamicsCommon.h>
#include <LinearMath/btVector3.h>
#include <LinearMath/btAlignedObjectArray.h>
#include <btBulletDynamicsCommon.h>
#include <scene_graph/scene_graph.hpp>

#define USE_MOTIONSTATE 1

namespace phys
{
	namespace util
	{
		inline btVector3 BV3toDXV3(DirectX::XMVECTOR v)
		{
			return btVector3(v.m128_f32[0], v.m128_f32[1], v.m128_f32[2]);
		}
		inline DirectX::XMVECTOR DXV3toBV3(btVector3 v)
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
		std::vector<btConvexHullShape*> CreateConvexShape(wr::Model* model);

		btRigidBody* CreateRigidBody(float mass, const btTransform& startTransform, btCollisionShape* shape);

		void UpdateSim(float delta, wr::SceneGraph& sg);

		void Destroy();
	};

} /* phys*/