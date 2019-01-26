#pragma once

#include <scene_graph/camera_node.hpp>
#include <windef.h>
#include <windowsx.h>

#include <btBulletDynamicsCommon.h>
#include <LinearMath/btVector3.h>
#include <LinearMath/btAlignedObjectArray.h>
#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include "util/log.hpp"
#include "physics_engine.hpp"
#include "phys_node.hpp"

class DebugCamera : public wr::CameraNode
{
public:
	btCapsuleShape* m_shape = nullptr;
	btRigidBody* m_rigid_body = nullptr;
	float m_height;
	bool m_grounded = false;
	float m_jump_power = 5;
	float m_shooting_power = 7;
	wr::SceneGraph* m_sg;
	float fire_delay = 0;
	float fire_rate = 0.5;
	float bullet_decay_delay = 0;
	float bullet_decay_rate = 2;
	std::vector<std::shared_ptr<PhysicsMeshNode>> m_bullets;
	std::vector<std::shared_ptr<PhysicsMeshNode>> m_active_bullets;
	int m_next_bullet = 0;

	//btPairCachingGhostObject* m_ghost;
	//btKinematicCharacterController* m_controller;

	DebugCamera(float fov, float aspect_ratio, wr::SceneGraph* sg, std::vector<std::shared_ptr<PhysicsMeshNode>> bullets)
		: wr::CameraNode(fov, aspect_ratio), m_forward_axis(0), m_right_axis(0), m_up_axis(0), m_rmb_down(false), m_lmb_down(false), m_speed(1), m_sensitivity(0.01), m_sg(sg), m_bullets(bullets)
	{
	}

	void SetupSimpleSphereColl(phys::PhysicsEngine& phys_engine, float radius, float height, float mass)
	{
		btTransform transform; 
		m_height = height;
		transform.setIdentity();

		m_shape = phys_engine.CreateCapsuleShape(radius, height);
		m_rigid_body = phys_engine.CreateRigidBody(btScalar(mass), transform, m_shape);
		m_rigid_body->setRestitution(0);
		m_rigid_body->setFriction(0);
		m_rigid_body->setRollingFriction(0);
		//m_rigid_body->Set
		m_rigid_body->activate(true);
		m_rigid_body->setDamping(0.95f, 0.5f);
		//body->setLinearFactor(btVector3(1, 0, 1));
		m_rigid_body->setAngularFactor(btVector3(0, 1, 0));
		//m_ghost = new btPairCachingGhostObject();
		//m_controller = new btKinematicCharacterController(m_ghost, m_shape, 1);
	}

	//Takes roll, pitch and yaw and converts it to quaternion
	virtual void SetRotation(DirectX::XMVECTOR roll_pitch_yaw) override
	{
		//auto quat = DirectX::XMQuaternionRotationRollPitchYawFromVector({});
		//auto& world_trans = m_rigid_body->getWorldTransform();
		//world_trans.setRotation(btQuaternion(quat.m128_f32[0], quat.m128_f32[1], quat.m128_f32[2], quat.m128_f32[3]));

		m_rotation_radians = roll_pitch_yaw;
	}

	//Sets position
	virtual void SetPosition(DirectX::XMVECTOR position) override
	{
		auto& world_trans = m_rigid_body->getWorldTransform();
		world_trans.setOrigin(phys::util::BV3toDXV3(position));

		m_position = position;
		SignalTransformChange();
	}

	virtual void SetSpeed(float speed)
	{
		m_speed = speed;
	}

	virtual void Update(phys::PhysicsEngine& phys_engine, wr::SceneGraph& sg, float delta)
	{
		POINT cursor_pos;
		GetCursorPos(&cursor_pos);
		
		btVector3 btTo = m_rigid_body->getWorldTransform().getOrigin() + (btVector3(0, 1, 0) * ((m_height+0.1)));
		btCollisionWorld::ClosestRayResultCallback res(m_rigid_body->getWorldTransform().getOrigin(), btTo);

		phys_engine.phys_world->rayTest(m_rigid_body->getWorldTransform().getOrigin(), btTo, res); // m_btWorld is btDiscreteDynamicsWorld

		if (fire_delay <= 0 && m_lmb_down)
		{
			Shoot();
		}

		if (bullet_decay_delay <= 0 && !m_active_bullets.empty())
		{
			bullet_decay_delay = bullet_decay_rate;
			// put bullets out of the screen.
			auto& b = m_active_bullets.front();
			b->SetPosition({ 0, 200, 0 });
			m_active_bullets.erase(m_active_bullets.begin()); // pop front
		}

		fire_delay -= delta;
		bullet_decay_delay -= delta;

		if (res.hasHit()) {
			m_grounded = true;
		}
		else
		{
 			m_grounded = false;
		}

		m_rigid_body->activate(true);

		// Translation
		m_right_axis = std::min(m_right_axis, 1.f);
		m_right_axis = std::max(m_right_axis, -1.f);

		m_forward_axis = std::min(m_forward_axis, 1.f);
		m_forward_axis = std::max(m_forward_axis, -1.f);

		m_up_axis = std::min(m_up_axis, 1.f);
		m_up_axis = std::max(m_up_axis, -1.f);

		DirectX::XMVECTOR forward = DirectX::XMVector3Normalize(m_transform.r[2]);
		DirectX::XMVECTOR up = DirectX::XMVector3Normalize(m_transform.r[1]);
		DirectX::XMVECTOR right = DirectX::XMVector3Normalize(m_transform.r[0]);

		DirectX::XMVECTOR translation = {0, 0, 0, 0};
		translation = DirectX::XMVectorAdd(translation, DirectX::XMVectorScale(forward, m_speed * m_forward_axis));
		//translation = DirectX::XMVectorAdd(translation, DirectX::XMVectorScale(up, m_speed * m_up_axis));
		translation = DirectX::XMVectorAdd(translation, DirectX::XMVectorScale(right, m_speed * m_right_axis));

		m_rigid_body->applyCentralForce(phys::util::BV3toDXV3(translation));

		SignalTransformChange();
	}

	void Shoot()
	{
		fire_delay = fire_rate;

		DirectX::XMVECTOR forward = DirectX::XMVector3Normalize(m_transform.r[2]);
		DirectX::XMVECTOR velocity = DirectX::XMVectorScale(forward, m_shooting_power);
		DirectX::XMVECTOR position = DirectX::XMVectorAdd(m_position, DirectX::XMVectorScale(forward, 0.5));

		auto& bullet = m_bullets[m_next_bullet];
		bullet->SetPosition(position);

		bullet->m_rigid_body->setLinearVelocity(phys::util::BV3toDXV3(velocity) + m_rigid_body->getLinearVelocity());

		if (m_active_bullets.empty())
		{
			bullet_decay_delay = bullet_decay_rate;
		}
		m_active_bullets.push_back(bullet);

		m_next_bullet = (m_next_bullet + 1) % m_bullets.size();
	}

	void MouseMove(float x, float y)
	{
		// Rotation
		DirectX::XMVECTOR new_rot{ y, x };
		m_rotation_radians = DirectX::XMVectorSubtract(m_rotation_radians, DirectX::XMVectorScale(new_rot, m_sensitivity));

		m_rotation_radians.m128_f32[0] = std::max(m_rotation_radians.m128_f32[0], DirectX::XMConvertToRadians(-80.f));
		m_rotation_radians.m128_f32[0] = std::min(m_rotation_radians.m128_f32[0], DirectX::XMConvertToRadians(80.f));

		SignalTransformChange();

	
	}

	void MouseAction(int key, int action, LPARAM l_param)
	{
		if (action == WM_RBUTTONDOWN)
		{
			m_rmb_down = true;
		}
		else if (action == WM_RBUTTONUP)
		{
			m_rmb_down = false;
		}
		if (action == WM_LBUTTONDOWN)
		{
			m_lmb_down = true;
		}
		else if (action == WM_LBUTTONUP)
		{
			m_lmb_down = false;
		}
	}

	const float m_scroll_speed = 0.25f;

	void MouseWheel(int amount)
	{
	}

	// Due to the lack of a input manager I cheat input like this.
	void KeyAction(int key, int action)
	{
		if (action == WM_KEYDOWN)
		{
			if (key == 0x57) // W
			{
				m_forward_axis += 1;
			}
			if (key == 0x53) // S
			{
				m_forward_axis += -1;
			}
			if (key == 0x44) // A
			{
				m_right_axis += -1;
			}
			if (key == 0x41) // S
			{
				m_right_axis += 1;
			}
			if (key == VK_SPACE)
			{
				if (m_grounded)
				{
					m_rigid_body->applyCentralImpulse(btVector3(0, -m_jump_power, 0));
					m_grounded = false;
				}
			}
		}
		
		else if (action == WM_KEYUP)
		{
			if (key == 0x57) // W
			{
				m_forward_axis -= 1;
			}
			if (key == 0x53) // S
			{
				m_forward_axis -= -1;
			}
			if (key == 0x44) // A
			{
				m_right_axis -= -1;
			}
			if (key == 0x41) // S
			{
				m_right_axis -= 1;
			}
		}
	}

private:
	bool m_rmb_down;
	bool m_lmb_down;
	float m_speed;
	float m_sensitivity;
	float m_forward_axis;
	float m_right_axis;
	float m_up_axis;
};