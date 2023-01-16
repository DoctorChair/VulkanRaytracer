#include "CameraController.h"
#include "glm/gtc/matrix_transform.hpp"

CameraController::CameraController(float translationSpeed, float rotationSensitivity, float fov, float aspectRatio, float nearPlane, float farPlane, glm::vec3 up)
{
	_projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
	_translationSpeed = translationSpeed;
	_rotationSpeed = rotationSensitivity;
	_up = up;
}

void CameraController::update(float deltaTime, float yawDelta, float pitchDelta, float deltaRight, float deltaFront)
{
	glm::vec2 deltaVector = glm::vec2(yawDelta, pitchDelta);

	if (glm::length(deltaVector) < 2.0f)
	{
		deltaVector = glm::vec2(0.0f);
	}
	_angles = _angles + deltaVector * _rotationSpeed * deltaTime;
	
	if (_angles.y >= 90.0f)
		_angles.y = 89.0f;
	if (_angles.y <= -90.0f)
		_angles.y = -89.0f;

	glm::vec3 front = glm::vec3(0.0f);
	front.x = cos(glm::radians(_angles.x)) * cos(glm::radians(_angles.y));
	front.y = sin(glm::radians(_angles.y));
	front.z = sin(glm::radians(_angles.x)) * cos(glm::radians(_angles.y));
	front = glm::normalize(front);

	glm::vec3 translateVector = front * deltaFront + (glm::normalize(glm::cross(front, _up)) * deltaRight);
	
	if (glm::length(translateVector) > 0.0f)
	{
		translateVector = glm::normalize(translateVector) * _translationSpeed * deltaTime;
		_position = glm::translate(glm::mat4(1.0f), translateVector) * glm::vec4(_position, 1.0f);
	}

	_viewMatrix = glm::lookAt(_position, _position + front, _up);
}
