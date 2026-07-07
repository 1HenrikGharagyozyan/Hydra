#!/bin/bash

# Get the absolute path to the scripts folder
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
# Navigate to the root of the Hydra project
cd "$SCRIPT_DIR/../.."

echo "=== Generating project files ==="
if ! premake5 gmake2; then
    echo "Project generation failed! Exiting..."
    exit 1
fi

echo "=== Building Hydra Engine (Debug) ==="
if ! make config=debug -j"$(nproc)"; then
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
