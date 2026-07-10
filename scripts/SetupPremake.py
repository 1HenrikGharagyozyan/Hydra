import sys
import os
import platform
from pathlib import Path

import Utils


class PremakeConfiguration:
    premakeVersion = "5.0.0-beta1"
    premakeDirectory = "./vendor/premake/bin"
    premakeLicenseUrl = "https://raw.githubusercontent.com/premake/premake-core/master/LICENSE.txt"

    premakeZipUrls = {
        "Windows": f"https://github.com/premake/premake-core/releases/download/v{premakeVersion}/premake-{premakeVersion}-windows.zip",
        "Linux":   f"https://github.com/premake/premake-core/releases/download/v{premakeVersion}/premake-{premakeVersion}-linux.tar.gz",
        "Darwin":  f"https://github.com/premake/premake-core/releases/download/v{premakeVersion}/premake-{premakeVersion}-macosx.tar.gz",
    }

    premakeExeNames = {
        "Windows": "premake5.exe",
        "Linux":   "premake5",
        "Darwin":  "premake5",
    }

    @classmethod
    def Validate(cls):
        if not cls.CheckIfPremakeInstalled():
            print("Premake is not installed.")
            return False

        print(f"Correct Premake located at {os.path.abspath(cls.premakeDirectory)}")
        return True

    @classmethod
    def CheckIfPremakeInstalled(cls):
        system = platform.system()
        exeName = cls.premakeExeNames.get(system, "premake5")
        premakeExe = Path(f"{cls.premakeDirectory}/{exeName}")
        if not premakeExe.exists():
            return cls.InstallPremake()
        return True

    @classmethod
    def InstallPremake(cls):
        system = platform.system()

        if system not in cls.premakeZipUrls:
            print(f"Unsupported platform: {system}. Please install Premake manually: https://premake.github.io")
            return False

        permissionGranted = False
        while not permissionGranted:
            reply = str(input(
                "Premake not found. Would you like to download Premake {0:s}? [Y/N]: ".format(cls.premakeVersion)
            )).lower().strip()[:1]
            if reply == 'n':
                return False
            permissionGranted = (reply == 'y')

        zipUrl = cls.premakeZipUrls[system]
        isZip = zipUrl.endswith(".zip")
        archiveExt = ".zip" if isZip else ".tar.gz"
        archiveName = f"premake-{cls.premakeVersion}-{'windows' if system == 'Windows' else 'linux' if system == 'Linux' else 'macosx'}{archiveExt}"
        premakePath = f"{cls.premakeDirectory}/{archiveName}"

        print("Downloading {0:s} to {1:s}".format(zipUrl, premakePath))
        Utils.DownloadFile(zipUrl, premakePath)

        print("Extracting", premakePath)
        if isZip:
            Utils.UnzipFile(premakePath, deleteZipFile=True)
        else:
            Utils.UntarFile(premakePath, deleteArchiveFile=True)

        # Make executable on Unix
        if system in ("Linux", "Darwin"):
            exePath = os.path.abspath(f"{cls.premakeDirectory}/premake5")
            os.chmod(exePath, 0o755)
            print(f"Set executable permissions on {exePath}")

        print(f"Premake {cls.premakeVersion} has been downloaded to '{cls.premakeDirectory}'")

        premakeLicensePath = f"{cls.premakeDirectory}/LICENSE.txt"
        print("Downloading {0:s} to {1:s}".format(cls.premakeLicenseUrl, premakeLicensePath))
        Utils.DownloadFile(cls.premakeLicenseUrl, premakeLicensePath)
        print(f"Premake License has been downloaded to '{cls.premakeDirectory}'")

        return True


if __name__ == "__main__":
    PremakeConfiguration.Validate()