#pragma once

#include <scene_graph/camera_node.hpp>
#include <windef.h>
#include <windowsx.h>

class DebugCamera : public wr::CameraNode
{
public:
	DebugCamera(float fov, float aspect_ratio)
		: wr::CameraNode(fov, aspect_ratio), m_forward_axis(0), m_right_axis(0), m_up_axis(0), m_rmb_down(false), m_speed(1), m_sensitivity(0.01), m_position_lerp_speed(10.f), m_rotation_lerp_speed(5.f)
	{
		GetCursorPos(&m_last_cursor_pos);
		m_target_rotation_euler = m_rotation_radians;
		m_target_position = m_position;
	}

	//Takes roll, pitch and yaw and converts it to quaternion
	virtual void SetRotation(DirectX::XMVECTOR roll_pitch_yaw) override
	{
		m_rotation_radians = roll_pitch_yaw;
		m_target_rotation_euler = roll_pitch_yaw;
	}

	//Sets position
	virtual void SetPosition(DirectX::XMVECTOR position) override
	{
		m_position = position;
		m_target_position = position;
	}

	virtual void Update(float delta)
	{
		POINT cursor_pos;
		GetCursorPos(&cursor_pos);

		if (m_rmb_down) {
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

			m_target_position = DirectX::XMVectorAdd(m_target_position, DirectX::XMVectorScale(forward, delta * m_speed * m_forward_axis));
			m_target_position = DirectX::XMVectorAdd(m_target_position, DirectX::XMVectorScale(up, delta * m_speed * m_up_axis));
			m_target_position = DirectX::XMVectorAdd(m_target_position, DirectX::XMVectorScale(right, delta * m_speed * m_right_axis));

			// Rotation
			DirectX::XMVECTOR new_rot{ cursor_pos.y - m_last_cursor_pos.y, cursor_pos.x - m_last_cursor_pos.x };
			m_target_rotation_euler = DirectX::XMVectorSubtract(m_target_rotation_euler, DirectX::XMVectorScale(new_rot, m_sensitivity));
		}
		else
		{
			m_forward_axis = 0;
			m_right_axis = 0;
			m_up_axis = 0;
		}

		m_position = DirectX::XMVectorLerp(m_position, m_target_position, delta * m_position_lerp_speed);
		m_rotation_radians = DirectX::XMVectorLerp(m_rotation_radians, m_target_rotation_euler, delta * m_rotation_lerp_speed);
		SignalTransformChange();

		m_last_cursor_pos = cursor_pos;
	}

	void MouseAction(int key, int action)
	{
		if (action == WM_RBUTTONDOWN)
		{
			m_rmb_down = true;
		}
		else if (action == WM_RBUTTONUP)
		{
			m_rmb_down = false;
		}
	}

	const float m_scroll_speed = 0.25f;

	void MouseWheel(int amount)
	{
		if (m_rmb_down)
		{
			float percent = (float) GET_WHEEL_DELTA_WPARAM(amount) / WHEEL_DELTA * m_scroll_speed;

			if (percent + m_speed > 0)
				m_speed += percent;
		}

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
				m_up_axis += -1;
			}
			if (key == VK_CONTROL)
			{
				m_up_axis += 1;
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
			if (key == VK_SPACE)
			{
				m_up_axis -= -1;
			}
			if (key == VK_CONTROL)
			{
				m_up_axis -= 1;
			}
		}
	}

private:
	float m_position_lerp_speed;
	float m_rotation_lerp_speed;
	DirectX::XMVECTOR m_target_rotation_euler;
	DirectX::XMVECTOR m_target_position;
	POINT m_last_cursor_pos;
	bool m_rmb_down;
	float m_speed;
	float m_sensitivity;
	float m_forward_axis;
	float m_right_axis;
	float m_up_axis;
};