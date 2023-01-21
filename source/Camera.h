#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		enum class CameraMode
		{
			Hardware,
			Software
		};

		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle, float _aspectRatio) :
			origin{ _origin },
			fovAngle{ _fovAngle },
			aspectRatio{ _aspectRatio }
		{
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);


			CalculateProjectionMatrix();
			CalculateViewMatrix();
		}


		void ToggleCameraMode() { cameraMode = static_cast<CameraMode>((static_cast<int>(cameraMode) + 1) % (static_cast<int>(CameraMode::Software) + 1)); }
		CameraMode cameraMode{ CameraMode::Hardware };

		Vector3 origin{};
		float fovAngle{ 45.f };
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };

		float aspectRatio{ 1 };

		const float nearPlane{ 0.1f };
		const float farPlane{ 100 };

		Vector3 forward{ Vector3::UnitZ };
		Vector3 up{ Vector3::UnitY };
		Vector3 right{ Vector3::UnitX };

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};
		Matrix projectionMatrix{};


		const float defaultMouseMoveSpeedSoftware{ 20 };
		const float defaultMouseMoveSpeedHardware{ 200 };
		const float defaultMoveSpeed{ 20 };

		float moveSpeed{ 20 };
		float mouseMoveSpeedSoftware{ 20 };
		float mouseMoveSpeedHardware{ 200 };
		const float rotationSpeed{ 5 * TO_RADIANS };

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = { 0.f,0.f,0.f }, float _aspectRatio = 1)
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;

			aspectRatio = _aspectRatio;


			CalculateProjectionMatrix();
			CalculateViewMatrix();

		}

		void CalculateViewMatrix()
		{
			//TODO W1
			//ONB => invViewMatrix
			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right);


			invViewMatrix = {
					right,
					up,
					forward,
					origin
			};
			//Inverse(ONB) => ViewMatrix
			viewMatrix = invViewMatrix.Inverse();

			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			//TODO W2

			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			projectionMatrix = Matrix::CreatePerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}


		void Update(const Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);


			if (pKeyboardState[SDL_SCANCODE_LSHIFT] || pKeyboardState[SDL_SCANCODE_RSHIFT])
			{
				moveSpeed = defaultMoveSpeed * 4;
				mouseMoveSpeedHardware = defaultMouseMoveSpeedHardware * 4;
				mouseMoveSpeedSoftware = defaultMouseMoveSpeedSoftware * 4;
			}
			else
			{
				moveSpeed = defaultMoveSpeed;
				mouseMoveSpeedHardware = defaultMouseMoveSpeedHardware;
				mouseMoveSpeedSoftware = defaultMouseMoveSpeedSoftware;
			}

			//WS for Forward-Backwards amount 
			origin += (pKeyboardState[SDL_SCANCODE_W] | pKeyboardState[SDL_SCANCODE_UP]) * moveSpeed * forward * deltaTime;
			origin += (pKeyboardState[SDL_SCANCODE_S] | pKeyboardState[SDL_SCANCODE_DOWN]) * moveSpeed * -forward * deltaTime;


			//DA for Left-Right amount				 
			origin += (pKeyboardState[SDL_SCANCODE_D] | pKeyboardState[SDL_SCANCODE_RIGHT]) * moveSpeed * right * deltaTime;
			origin += (pKeyboardState[SDL_SCANCODE_A] | pKeyboardState[SDL_SCANCODE_LEFT]) * moveSpeed * -right * deltaTime;

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			mouseX = Clamp(mouseX, -1, 1);
			mouseY = Clamp(mouseY, -1, 1);
			switch (mouseState)
			{
			case SDL_BUTTON_LMASK:

				switch (cameraMode)
				{
				case Camera::CameraMode::Hardware:
					origin -= mouseY * mouseMoveSpeedHardware * forward * deltaTime;
					break;
				case Camera::CameraMode::Software:
					origin -= mouseY * mouseMoveSpeedSoftware * forward * deltaTime;
					break;
				}

				totalYaw += mouseX * rotationSpeed;
				break;
			case SDL_BUTTON_RMASK:
				totalYaw += mouseX * rotationSpeed;
				totalPitch -= mouseY * rotationSpeed;
				break;
			case SDL_BUTTON_X2:
				switch (cameraMode)
				{
				case Camera::CameraMode::Hardware:
					origin += mouseY * mouseMoveSpeedHardware * deltaTime * up;
					break;
				case Camera::CameraMode::Software:
					origin += mouseY * mouseMoveSpeedSoftware * deltaTime * up;
					break;
				}
				break;
			}


			const Matrix pitchMatrix{ Matrix::CreateRotationX(totalPitch * TO_RADIANS) };
			const Matrix yawMatrix{ Matrix::CreateRotationY(totalYaw * TO_RADIANS) };
			const Matrix rollMatrix{ Matrix::CreateRotationZ(0) };
				  
			const Matrix rotationMatrix{ pitchMatrix * yawMatrix * rollMatrix };

			forward = rotationMatrix.TransformVector(Vector3::UnitZ);
			//Update Matrices
			CalculateViewMatrix();
		}

	};
}

