#!/usr/bin/env python3
"""
Universal entry point for project setup.
Run this from anywhere and it will check/install Python, Premake, Vulkan
and Mono dependencies, pull submodules, generate project files, and (on
Linux) build and launch Hydra straight away.

Usage:
  Windows : python Setup.py
  Linux   : python3 Setup.py
  macOS   : python3 Setup.py
"""

import subprocess
import sys
import os

if __name__ == "__main__":
    rootDir = os.path.dirname(os.path.abspath(__file__))
    setup_script = os.path.join(rootDir, "scripts", "Setup.py")

    result = subprocess.call([sys.executable, setup_script])
    sys.exit(result)

