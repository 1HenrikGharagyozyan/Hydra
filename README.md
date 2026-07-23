<div align="center">

<br/>

# Hydra Engine

**An early-stage interactive application and rendering engine**

![Platform](https://img.shields.io/badge/Platform-Linux-informational?style=flat-square&logo=linux&logoColor=white&color=0d1117)
![Graphics](https://img.shields.io/badge/Graphics-Vulkan-red?style=flat-square&logo=vulkan&logoColor=white&color=AC1A1A)
![Language](https://img.shields.io/badge/Language-C%2B%2B-blue?style=flat-square&logo=cplusplus&logoColor=white&color=00599C)
![Status](https://img.shields.io/badge/Status-Early--Stage-yellow?style=flat-square&color=E6A817)

<br/>

</div>

---

Hydra is an early-stage interactive application and rendering engine currently developed on **Linux**, with Windows and cross-platform support planned for the future. Everything inside this repository is being developed progressively as the engine evolves, with a strong focus on **clean system design**, **performance**, and **extensibility**.

---

## 🚀 Getting Started

Clone the repository using Git, making sure to include all submodules:

```bash
git clone --recursive https://github.com/1HenrikGharagyozyan/Hydra.git
```

> ⚠️ The `--recursive` flag is **required** to fetch all submodules.

---

## 🐧 Linux (Ubuntu / Debian)

Hydra supports Linux natively via Makefiles and requires the **Vulkan SDK** for shader compilation (SPIR-V).

### Prerequisites

Ensure you have the Vulkan SDK installed (from the LunarG repository), along with standard build tools:

```bash
sudo apt update
sudo apt install build-essential python3 vulkan-sdk
```

### Build Steps

Run the setup script from the repo root. It checks/installs Premake, the
Vulkan SDK and Mono, pulls submodules, generates Makefiles via Premake, and
then builds and launches HydraEditor automatically — no further steps
needed:

```bash
python3 Setup.py
```

If you only want to (re)generate and build without launching the editor,
you can run the underlying pieces yourself instead:

```bash
python3 scripts/Setup.py          # dependency checks + submodules only
./scripts/Launch/Linux-GenProject.sh   # generate, build, and launch
```

or, for finer-grained control:

```bash
./vendor/premake/bin/premake5 gmake2   # generate Makefiles
make -j$(nproc)                        # build the engine and applications
./bin/Debug-linux-x86_64/HydraEditor/HydraEditor   # launch the editor
```

---

## 📦 Recent Updates

| Area | Description |
|------|-------------|
| 🎨 **Rendering** | Implemented a modern shader system with SPIR-V cross-compilation and Uniform Buffer Objects (UBOs) for optimized rendering |
| 🔧 **Build System** | Improved Premake configuration for Linux and added Python/Bash scripts for automated dependency checks and project generation |
| ⚙️ **Core** | Added command-line argument parsing |

---

## 🗺️ Roadmap

The goal of Hydra is to build a **powerful, scalable 3D engine** with modern architecture — expanding capabilities while maintaining a clean codebase.

**Planned features include:**

- [ ] Fast 2D rendering (UI, particles, sprites, etc.)
- [ ] High-fidelity Physically-Based 3D rendering
- [ ] Official support for Windows, Mac, Android, and iOS
- [ ] Native rendering API support (DirectX, Vulkan, Metal)
- [ ] Fully featured viewer and editor applications
- [ ] Fully scripted interaction and behavior
- [ ] Integrated 2D and 3D physics engine
- [ ] Procedural terrain and world generation
- [ ] Artificial Intelligence systems
- [ ] Audio system

---

<div align="center">

*Hydra is under active development — built progressively, designed to last.*

</div>