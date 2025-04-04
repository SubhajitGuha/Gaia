#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include "Camera.h"
#include "Gaia/Events/ApplicationEvent.h"
#include "Gaia/Events/KeyEvent.h"
#include "Gaia/Events/MouseEvent.h"

namespace Gaia {
	class EditorCamera :public Camera
	{
	public:
		static float camera_MovementSpeed;
		EditorCamera();
		EditorCamera(float width, float Height);
		~EditorCamera() = default;

		void SetPerspctive(float v_FOV,float Near,float Far) override;
		void SetCameraPosition(const glm::vec3& pos) override { m_Position = pos; RecalculateProjectionView();}
		void SetViewDirection(const glm::vec3& dir) override { m_ViewDirection = dir; RecalculateProjectionView(); }
		void SetUPVector(const glm::vec3& up) override {	Up = up; RecalculateProjectionView();}
		void SetViewportSize(float aspectratio) override;
		void SetVerticalFOV(float fov) override { m_verticalFOV = fov; SetPerspctive(m_verticalFOV, m_PerspectiveNear, m_PerspectiveFar); }
		void SetPerspectiveNear(float val) override { m_PerspectiveNear = val; SetPerspctive(m_verticalFOV, m_PerspectiveNear, m_PerspectiveFar);}
		void SetPerspectiveFar(float val) override { m_PerspectiveFar = val; SetPerspctive(m_verticalFOV, m_PerspectiveNear, m_PerspectiveFar);}

		float GetPerspectiveNear() override { return m_PerspectiveNear; };
		float GetPerspectiveFar() override { return m_PerspectiveFar; };
		glm::mat4 GetProjectionView() override { return m_ProjectionView; }
		inline glm::mat4 GetViewMatrix() override { return m_View; }
		inline glm::mat4 GetProjectionMatrix() override { return m_Projection; }
		inline glm::vec3 GetCameraPosition() override { return m_Position; }
		inline glm::vec3 GetCameraRotation() override { return glm::vec3(m_pitch, m_yaw, m_roll); };
		inline glm::vec3 GetViewDirection() override { return m_ViewDirection; }
		inline float GetAspectRatio() override { return m_AspectRatio; }
		inline float GetVerticalFOV() override { return m_verticalFOV; }

		void OnEvent(Event& e) override;
		void OnUpdate(TimeStep ts) override;
		void RotateCamera(float pitch = 0, float yaw = 0, float roll = 0) override;
	private:
		void RecalculateProjection();
		void RecalculateProjectionView();

	private:
		glm::mat4 m_View;
		glm::mat4 m_Projection;
		glm::mat4 m_ProjectionView;

		glm::vec3 m_Position = { 0,0,-10 }, m_ViewDirection = { 0,0,1 };
		//m_Viewdirection is the location we are looking at (it is the vector multiplied with rotation matrix)
		glm::vec3 Up = { 0,1,0 } , RightVector;//we get right vector by getting the cross product of m_ViewDirection and Up vectors

		float m_verticalFOV = 45.f;
		float m_PerspectiveNear = 1.0;
		float m_PerspectiveFar = 1000;
		
		float m_AspectRatio = 1.0;
		float m_pitch=0, m_yaw=0, m_roll=0;
		//camera parameters
		float m_movespeed = 20;
		glm::vec2 OldMousePos = { 0,0};
	};
}