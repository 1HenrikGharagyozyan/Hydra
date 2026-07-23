import os
import platform
import subprocess
from pathlib import Path

import Utils

class MonoConfiguration:
    # Версия для скачивания на Windows
    monoVersion = "6.12.0.199" 
    monoInstallUrl = f"https://download.mono-project.com/archive/{monoVersion}/windows-installer/mono-{monoVersion}-x64-0.msi"
    monoInstallerPath = "./vendor/MonoInstaller.msi"

    @classmethod
    def Validate(cls):
        if not cls.CheckMono():
            print("Mono is not installed or configured correctly.")
            return False
        return True

    @classmethod
    def CheckMono(cls):
        system = platform.system()

        if system == "Windows":
            # Проверяем дефолтный путь установки Mono на Windows
            monoPath = Path("C:/Program Files/Mono/bin/mono.exe")
            if not monoPath.exists():
                print("\nCould not find Mono installed at C:/Program Files/Mono")
                return cls.__InstallMono(system)
            
            print(f"Correct Mono located at {monoPath.parent.parent}")
            return True

        elif system == "Linux":
            # На Linux лучший способ проверить библиотеки для C++ - использовать pkg-config
            try:
                result = subprocess.run(["pkg-config", "--exists", "mono-2"], capture_output=True)
                if result.returncode == 0:
                    print("Correct Mono (libmono-2.0-dev) located via pkg-config")
                    return True
                else:
                    print("\nMono development packages are missing!")
                    return cls.__InstallMono(system)
            except FileNotFoundError:
                print("\npkg-config is not installed!")
                return cls.__InstallMono(system)

        return False

    @classmethod
    def __InstallMono(cls, system):
        permissionGranted = False
        while not permissionGranted:
            reply = str(input("Would you like to install Mono? [Y/N]: ")).lower().strip()[:1]
            if reply == 'n':
                return False
            permissionGranted = (reply == 'y')

        if system == "Windows":
            print(f"Downloading {cls.monoInstallUrl} to {cls.monoInstallerPath}")
            Utils.DownloadFile(cls.monoInstallUrl, cls.monoInstallerPath)
            print("Running Mono installer...")
            os.startfile(os.path.abspath(cls.monoInstallerPath))
            print("Please follow the installer instructions.")
            print("Note: Make sure to install it to the default directory (C:/Program Files/Mono)")
            print("Re-run this script after installation!")
            quit()

        elif system == "Linux":
            print("\nOn Linux, please install Mono manually via your package manager:")
            print("  Ubuntu/Debian: sudo apt update && sudo apt install libmono-2.0-dev mono-runtime")
            print("  Arch Linux:    sudo pacman -S mono")
            print("\nRe-run this script after installation!")
            quit()

        else:
            print(f"Unsupported platform: {system}")
            print("Please install Mono manually.")

        return True

if __name__ == "__main__":
    MonoConfiguration.Validate()