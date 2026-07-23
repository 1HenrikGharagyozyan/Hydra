import os
import sys
import platform
import subprocess
from pathlib import Path

import Utils

from io import BytesIO
from urllib.request import urlopen


class VulkanConfiguration:
    requiredVulkanVersion = "1.3"
    installVulkanVersion = "1.3.216.0"
    vulkanDirectory = "./Hydra/vendor/VulkanSDK"

    @classmethod
    def Validate(cls):
        if not cls.CheckVulkanSDK():
            print("Vulkan SDK not installed correctly.")
            return

        if not cls.CheckVulkanSDKDebugLibs():
            print("\nNo Vulkan SDK debug libs found. Install Vulkan SDK with debug libs.")
            print("(see docs.hydraengine.com/GettingStarted for more info).")
            print("Debug configuration disabled.")

    @classmethod
    def CheckVulkanSDK(cls):
        vulkanSDK = os.environ.get("VULKAN_SDK")
        if vulkanSDK is None:
            print("\nYou don't have the Vulkan SDK installed!")
            cls.__InstallVulkanSDK()
            return False
        else:
            print(f"\nLocated Vulkan SDK at {vulkanSDK}")

        # Игнорируем жесткую проверку пути на Linux, так как SDK лежит в /usr
        if platform.system() == "Windows" and cls.requiredVulkanVersion not in vulkanSDK:
            print(f"You don't have the correct Vulkan SDK version! "
                  f"(Engine requires {cls.requiredVulkanVersion})")
            cls.__InstallVulkanSDK()
            return False

        print(f"Correct Vulkan SDK located at {vulkanSDK}")
        return True

    @classmethod
    def __InstallVulkanSDK(cls):
        permissionGranted = False
        while not permissionGranted:
            reply = str(input(
                "Would you like to install VulkanSDK {0:s}? [Y/N]: ".format(cls.installVulkanVersion)
            )).lower().strip()[:1]
            if reply == 'n':
                return
            permissionGranted = (reply == 'y')

        system = platform.system()

        if system == "Windows":
            vulkanInstallURL = (
                f"https://sdk.lunarg.com/sdk/download/{cls.requiredVulkanVersion}"
                f"/windows/VulkanSDK-{cls.requiredVulkanVersion}-Installer.exe"
            )
            vulkanPath = f"{cls.vulkanDirectory}/VulkanSDK-{cls.requiredVulkanVersion}-Installer.exe"
            print(f"Downloading {vulkanInstallURL} to {vulkanPath}")
            Utils.DownloadFile(vulkanInstallURL, vulkanPath)
            print("Running Vulkan SDK installer...")
            os.startfile(os.path.abspath(vulkanPath))

        elif system == "Linux":
            print("\nOn Linux, please install the Vulkan SDK manually:")
            print("  Ubuntu/Debian: sudo apt install vulkan-sdk")
            print("  Or visit: https://vulkan.lunarg.com/sdk/home#linux")
            print(f"  Required version: {cls.requiredVulkanVersion}")

        elif system == "Darwin":
            vulkanInstallURL = (
                f"https://sdk.lunarg.com/sdk/download/{cls.installVulkanVersion}"
                f"/mac/vulkansdk-macos-{cls.installVulkanVersion}.dmg"
            )
            vulkanPath = f"{cls.vulkanDirectory}/vulkansdk-macos-{cls.installVulkanVersion}.dmg"
            print(f"Downloading {vulkanInstallURL} to {vulkanPath}")
            Utils.DownloadFile(vulkanInstallURL, vulkanPath)
            print("Mounting DMG installer...")
            subprocess.call(["hdiutil", "attach", os.path.abspath(vulkanPath)])
            print("Please follow the installer instructions.")

        else:
            print(f"Unsupported platform: {system}")
            print("Please install Vulkan SDK manually: https://vulkan.lunarg.com/sdk/home")

        print("Re-run this script after installation!")
        quit()

    @classmethod
    def CheckVulkanSDKDebugLibs(cls):
        system = platform.system()
        vulkanSDK = os.environ.get("VULKAN_SDK")

        if not vulkanSDK:
            return False

        if system == "Windows":
            shadercdLib = Path(f"{vulkanSDK}/Lib/shaderc_sharedd.lib")
        elif system == "Linux":
            shadercdLib = Path(f"{vulkanSDK}/lib/libshaderc_sharedd.so")
        elif system == "Darwin":
            shadercdLib = Path(f"{vulkanSDK}/lib/libshaderc_sharedd.dylib")
        else:
            return False

        return shadercdLib.exists()


if __name__ == "__main__":
    VulkanConfiguration.Validate()