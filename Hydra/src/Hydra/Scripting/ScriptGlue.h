#pragma once

namespace Hydra
{

	// Registers the C++ functions C# scripts are allowed to call
	// (via mono_add_internal_call). Must run after mono_jit_init() and
	// before any managed method that references one of them is invoked.
	class ScriptGlue
	{
	public:
		static void RegisterFunctions();
	};

}
