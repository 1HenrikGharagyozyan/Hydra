#pragma once

#include "Hydra/Core/KeyCodes.h"
#include "Hydra/Core/MouseCodes.h"

#include <glm/glm.hpp>


namespace Hydra
{
	
	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);

		static bool IsMouseButtonPressed(MouseCode button);
		static glm::vec2 GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
	};

}