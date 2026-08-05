#!/bin/bash

# Get the absolute path to this script's own directory (SandboxProject/Assets/Scripts)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Same relative depth to the repo root as HydraRootDir in premake5.lua
REPO_ROOT="$( cd "$SCRIPT_DIR/../../../.." && pwd )"
PREMAKE="$REPO_ROOT/vendor/premake/bin/premake5"

echo "=== Generating SandboxProject Makefiles ==="
if ! "$PREMAKE" gmake2; then
    echo "Project generation failed! Exiting..."
    exit 1
fi

# This is a standalone C# workspace (Sandbox links against Hydra-ScriptCore),
# built via the premake-generated Makefile with mcs/csc - not an SDK-style
# dotnet project. Ubuntu's mono packages only ship mcs, not csc.
if command -v csc &> /dev/null; then
    CSHARP_COMPILER=csc
elif command -v mcs &> /dev/null; then
    CSHARP_COMPILER=mcs
else
    echo "No C# compiler (csc/mcs) found. Install it with: sudo apt install mono-complete"
    exit 1
fi

echo "=== Building Sandbox (Debug) ==="
if ! make config=debug CSC="$CSHARP_COMPILER" -j"$(nproc)"; then
    echo "Build failed! Exiting..."
    exit 1
fi

echo "=== Build complete ==="
echo "Sandbox.dll:          $SCRIPT_DIR/Binaries/Sandbox.dll"
# Hydra-ScriptCore's targetdir is fixed relative to its own directory (not
# %{wks.location}), so it always builds to the same place regardless of which
# workspace (this one or the main engine's) triggered the generation - this
# is also what HydraEditor actually loads at runtime.
echo "Hydra-ScriptCore.dll: $REPO_ROOT/HydraEditor/Resources/Scripts/Hydra-ScriptCore.dll"
