#include "phys_node.hpp"
#include "physics_engine.hpp"

void PhysicsMeshNode::SetMass(float mass)
{
	m_mass = mass;

	if (m_rigid_body)
	{
		btVector3 local_intertia;
		m_shape->calculateLocalInertia(mass, local_intertia);
		m_rigid_body->setMassProps(mass, local_intertia);
	}
}

void PhysicsMeshNode::SetRestitution(float value)
{
	if (m_rigid_bodys.has_value())
	{
		for (auto& body : m_rigid_bodys.value())
		{
			body->setRestitution(value);
		}
	}
	else
	{
		m_rigid_body->setRestitution(value);
	}
}

void PhysicsMeshNode::SetupSimpleBoxColl(phys::PhysicsEngine& phys_engine, DirectX::XMVECTOR scale)
{
	btTransform transform;
	transform.setIdentity();

	m_shape = phys_engine.CreateBoxShape(phys::util::BV3toDXV3(scale));
	m_rigid_body = phys_engine.CreateRigidBody(btScalar(m_mass), transform, m_shape);
}

void PhysicsMeshNode::SetupSimpleSphereColl(phys::PhysicsEngine& phys_engine, float radius)
{
	btTransform transform;
	transform.setIdentity();

	m_shape = phys_engine.CreateSphereShape(radius);
	m_rigid_body = phys_engine.CreateRigidBody(btScalar(m_mass), transform, m_shape);
}

void PhysicsMeshNode::SetupConvex(phys::PhysicsEngine & phys_engine, wr::Model * model)
{
	m_rigid_bodys = std::vector<btRigidBody*>();
	m_shapes = std::vector<btCollisionShape*>();

	auto hulls = phys_engine.CreateConvexShape(model);

	for (auto& hull : hulls)
	{
		m_shapes->push_back(hull);

		btTransform transform;
		transform.setIdentity();
		auto body = phys_engine.CreateRigidBody(0.f, transform, hull);
		m_rigid_bodys->push_back(body);
	}
}

void PhysicsMeshNode::SetPosition(DirectX::XMVECTOR position)
{
	if (m_rigid_bodys.has_value())
	{
		for (auto& body : m_rigid_bodys.value())
		{
			auto& world_trans = body->getWorldTransform();
			world_trans.setOrigin(phys::util::BV3toDXV3(position));
		}
	}
	else
	{
		auto& world_trans = m_rigid_body->getWorldTransform();
		world_trans.setOrigin(phys::util::BV3toDXV3(position));
	}

	m_position = position;
	SignalTransformChange();
}

void PhysicsMeshNode::SetRotation(DirectX::XMVECTOR roll_pitch_yaw)
{
	auto quat = DirectX::XMQuaternionRotationRollPitchYawFromVector(roll_pitch_yaw);
	if (m_rigid_bodys.has_value())
	{
		for (auto& body : m_rigid_bodys.value())
		{
			auto& world_trans = body->getWorldTransform();
			world_trans.setRotation(btQuaternion(quat.m128_f32[0], quat.m128_f32[1], quat.m128_f32[2], quat.m128_f32[3]));
		}
	}
	else
	{
		auto& world_trans = m_rigid_body->getWorldTransform();
		world_trans.setRotation(btQuaternion(quat.m128_f32[0], quat.m128_f32[1], quat.m128_f32[2], quat.m128_f32[3]));
	}

	m_rotation_radians = roll_pitch_yaw;
	SignalTransformChange();
}

void PhysicsMeshNode::SetScale(DirectX::XMVECTOR scale)
{
	if (m_rigid_bodys.has_value())
	{
		for (auto& shape : m_shapes.value())
		{
			shape->setLocalScaling(phys::util::BV3toDXV3(scale));
		}
	}
	else
	{
		m_shape->setLocalScaling(phys::util::BV3toDXV3(scale));
	}

	m_scale = scale;
	SignalTransformChange();
}

void PhysicsMeshNode::Update(uint32_t frame_idx)
{
	wr::MeshNode::Update(frame_idx);
}