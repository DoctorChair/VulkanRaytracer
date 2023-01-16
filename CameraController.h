#pragma once
#include "glm/glm.hpp"

class CameraController
{
public:
	CameraController(float translationSpeed, float rotationSensitivity, float fov, float aspectRatio, float nearPlane, float farPlane, glm::vec3 up);
	void update(float deltaTime, float yawDelta, float tiltDelta, float deltaRight, float deltaFront);
	
	glm::mat4 _viewMatrix = glm::mat4(1.0f);
	glm::mat4 _projectionMatrix = glm::mat4(1.0f);
	glm::vec3 _position = glm::vec3(0.0f);

private:
	float _translationSpeed = 0;
	float _rotationSpeed = 0;

	glm::vec2 _angles = glm::vec2(0.0f);
	
	glm::vec3 _up;

	float _fov = 0;
	float _aspectRatio = 0;
	float _nearPlane = 0;
	float _farPlane = 0;

	
	
};