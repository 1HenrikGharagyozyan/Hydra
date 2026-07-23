import os
import subprocess
import platform
import sys

from SetupPython import PythonConfiguration as PythonRequirements

# Make sure everything we need for the setup is installed
PythonRequirements.Validate()

from SetupPremake import PremakeConfiguration as PremakeRequirements
from SetupVulkan import VulkanConfiguration as VulkanRequirements
from SetupMono import MonoConfiguration as MonoRequirements

# Resolve paths from this file's location, not the caller's cwd, so the
# script behaves the same whether it's run as `python3 scripts/Setup.py`
# from the repo root, `python Setup.py` from inside scripts/, or via the
# root-level Setup.py wrapper.
scriptsDir = os.path.dirname(os.path.abspath(__file__))
rootDir = os.path.abspath(os.path.join(scriptsDir, ".."))
os.chdir(rootDir)

premakeInstalled = PremakeRequirements.Validate()
VulkanRequirements.Validate()
MonoRequirements.Validate()

print("\nUpdating submodules...")
subprocess.call(["git", "submodule", "update", "--init", "--recursive"])

if not premakeInstalled:
    print("\nHydra requires Premake to generate project files.")
    sys.exit(1)

system = platform.system()

if system == "Windows":
    genScript = os.path.join(scriptsDir, "Launch", "Win-GenProjects.bat")
    print("\nGenerating Visual Studio project files (Windows)...")
    subprocess.call([genScript, "nopause"])
    print("\nSetup completed! Open Hydra.sln in Visual Studio to build and run the engine.")

elif system == "Linux":
    genScript = os.path.join(scriptsDir, "Launch", "Linux-GenProject.sh")
    print("\nGenerating Makefiles, building, and launching Hydra (Linux)...")
    subprocess.call(["bash", genScript])
    print("\nSetup completed!")

elif system == "Darwin":
    premakePath = os.path.join(rootDir, "vendor", "premake", "bin", "premake5")
    if os.path.exists(premakePath):
        print("\nRunning premake (macOS)...")
        subprocess.call([premakePath, "xcode4"])
        print("\nSetup completed! Open Hydra.xcworkspace in Xcode to build and run the engine.")
    else:
        print(f"ERROR: premake5 not found at {premakePath}")

else:
    print(f"\nUnknown platform: {system}. Skipping project generation.")