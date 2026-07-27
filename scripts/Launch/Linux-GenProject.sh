#!/bin/bash

# Get the absolute path to the scripts folder
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
# Navigate to the root of the Hydra project
cd "$SCRIPT_DIR/../.."

# Hydra-ScriptCore is a Premake/Mono project (built via the generated Makefile
# below), not an SDK-style project - there is no .csproj, so msbuild/dotnet
# build cannot target it. Pick whichever Mono C# compiler is actually
# installed; Ubuntu's mono packages only ship `mcs`, not `csc`.
if command -v csc &> /dev/null; then
    CSHARP_COMPILER=csc
elif command -v mcs &> /dev/null; then
    CSHARP_COMPILER=mcs
else
    echo "No C# compiler (csc/mcs) found. Install it with: sudo apt install mono-complete"
    exit 1
fi

# Инкрементальная сборка C++ и C# (пересобирает ТОЛЬКО измененные файлы)
echo "=== Building Hydra Engine (Debug) ==="
if ! make config=debug CSC="$CSHARP_COMPILER" -j"$(nproc)"; then
    echo "Build failed! Exiting..."
    exit 1
fi

echo "=== Starting Hydra Editor ==="

# Enter the editor working directory for correct asset paths
pushd HydraEditor > /dev/null

# Run the editor
./../bin/Debug-linux-x86_64/HydraEditor/HydraEditor

popd > /dev/null

echo "=== Hydra Editor exited ==="