#!/usr/bin/env python
"""
SConstruct for godot-plateau GDExtension
Build with: scons platform=<windows|linux|macos|android|ios|visionos> target=<template_debug|template_release|editor>

Android build requires:
  - ANDROID_HOME or ANDROID_SDK_ROOT environment variable set
  - NDK installed (default version: 23.1.7779620)
  - Example: scons platform=android arch=arm64

iOS build requires:
  - macOS with Xcode installed
  - Example: scons platform=ios arch=arm64

visionOS build requires:
  - macOS with Xcode 15+ installed
  - visionOS SDK
  - Example: scons platform=visionos arch=arm64
  - Simulator: scons platform=visionos arch=arm64 visionos_simulator=yes
"""

import os
import sys
import platform as python_platform
import shutil
import subprocess
from pathlib import Path

from SCons.Script import Dir

from methods import print_error, print_warning

# Configuration
LIB_NAME = "godot-plateau"
GODOT_PROJECT_DIR = "demo"

REPO_ROOT = Path(Dir('#').abspath)
LIBPLATEAU_ROOT = REPO_ROOT / "libplateau"
BUILD_ROOT = REPO_ROOT / "build"

# Platform-specific libplateau build settings
def get_libplateau_build_dir(platform, arch=""):
    """Get the libplateau build directory for the platform."""
    if platform in ("android", "ios", "visionos") and arch:
        return BUILD_ROOT / platform / arch / "libplateau"
    return BUILD_ROOT / platform / "libplateau"


def get_libplateau_lib_path(platform, build_dir, build_type):
    """Get the libplateau library path for the platform."""
    if platform == "windows":
        # Windows uses build type as subdirectory (Release/RelWithDebInfo/Debug)
        return build_dir / "src" / build_type / "plateau_combined.lib"
    elif platform == "macos":
        return build_dir / "src" / "libplateau.a"
    elif platform == "android":
        return build_dir / "src" / "libplateau.a"
    elif platform == "ios":
        return build_dir / "src" / "libplateau.a"
    elif platform == "visionos":
        return build_dir / "src" / "libplateau.a"
    else:  # linux
        return build_dir / "src" / "libplateau_combined.a"


def get_cmake_build_type(target_name):
    """Map SCons target to CMake build type."""
    target_to_cmake = {
        "editor": "RelWithDebInfo",
        "template_debug": "RelWithDebInfo",
        "template_release": "Release",
    }
    return target_to_cmake.get(target_name, "Release")


def get_cmake_configure_args(platform, build_dir, build_type, env=None):
    """Get platform-specific cmake configure arguments."""
    common_args = [
        "-DPLATEAU_USE_FBX=OFF",
        "-DPLATEAU_USE_HTTP=OFF",  # Disable HTTP/OpenSSL - use Godot's HTTPRequest instead
        "-DBUILD_LIB_TYPE=static",
        f"-DCMAKE_BUILD_TYPE:STRING={build_type}",
    ]

    if platform == "windows":
        return common_args + [
            "-DRUNTIME_LIB_TYPE=MT",  # Static runtime to match godot-cpp
            "-G", "Visual Studio 17 2022",
        ]
    elif platform == "macos":
        return common_args + [
            "-DRUNTIME_LIB_TYPE=MD",
            '-DCMAKE_CXX_FLAGS=-w',
            "-DCMAKE_OSX_ARCHITECTURES:STRING=arm64",
            "-G", "Ninja",
        ]
    elif platform == "android":
        # Get NDK path from environment
        ndk_version = env.get("ndk_version", "23.1.7779620") if env else "23.1.7779620"
        android_home = env.get("ANDROID_HOME", os.environ.get("ANDROID_HOME", os.environ.get("ANDROID_SDK_ROOT", ""))) if env else os.environ.get("ANDROID_HOME", os.environ.get("ANDROID_SDK_ROOT", ""))
        ndk_root = os.path.join(android_home, "ndk", ndk_version) if android_home else os.environ.get("ANDROID_NDK_ROOT", "")
        toolchain_file = os.path.join(ndk_root, "build", "cmake", "android.toolchain.cmake")

        # Map godot-cpp arch to Android ABI
        arch = env.get("arch", "arm64") if env else "arm64"
        android_abi_map = {
            "arm64": "arm64-v8a",
            "arm32": "armeabi-v7a",
            "x86_64": "x86_64",
            "x86_32": "x86",
        }
        android_abi = android_abi_map.get(arch, "arm64-v8a")

        return common_args + [
            "-G", "Ninja",
            "-DANDROID_PLATFORM=android-24",
            f"-DANDROID_ABI={android_abi}",
            f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}",
        ]
    elif platform == "ios":
        arch = env.get("arch", "arm64") if env else "arm64"
        ios_arch = "arm64" if arch in ("arm64", "universal") else arch
        return common_args + [
            "-G", "Ninja",
            "-DCMAKE_SYSTEM_NAME=iOS",
            f"-DCMAKE_OSX_ARCHITECTURES={ios_arch}",
            "-DCMAKE_OSX_DEPLOYMENT_TARGET=13.0",
            "-DCMAKE_C_FLAGS=-DPNG_ARM_NEON_OPT=0",
        ]
    elif platform == "visionos":
        # Use ios.toolchain.cmake with PLATFORM=VISIONOS
        toolchain_file = str(REPO_ROOT / "godot-cpp" / "cmake" / "ios.toolchain.cmake")
        visionos_simulator = env.get("visionos_simulator", False) if env else False
        visionos_platform = "SIMULATOR_VISIONOS" if visionos_simulator else "VISIONOS"
        return common_args + [
            "-G", "Ninja",
            f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}",
            f"-DPLATFORM={visionos_platform}",
            "-DDEPLOYMENT_TARGET=1.0",
            "-DIOS=ON",  # Treat as iOS to enable mobile dummy implementations
        ]
    else:  # linux
        return common_args


def configure_libplateau(target, source, env):
    """Configure libplateau with cmake."""
    cmake_executable = shutil.which("cmake")
    if not cmake_executable:
        raise RuntimeError("cmake executable not found in PATH.")

    platform = env["platform"]
    arch = env.get("arch", "")
    build_type = env.get("LIBPLATEAU_BUILD_TYPE", "Release")
    build_dir = get_libplateau_build_dir(platform, arch)

    cmake_args = [
        cmake_executable,
        "-S", str(LIBPLATEAU_ROOT),
        "-B", str(build_dir),
    ] + get_cmake_configure_args(platform, build_dir, build_type, env)

    print(f"Configuring libplateau: {' '.join(cmake_args)}")
    subprocess.check_call(cmake_args)

    target_path = Path(str(target[0]))
    target_path.parent.mkdir(parents=True, exist_ok=True)
    target_path.touch()
    return 0


def build_libplateau(target, source, env):
    """Build libplateau with cmake."""
    cmake_executable = shutil.which("cmake")
    if not cmake_executable:
        raise RuntimeError("cmake executable not found in PATH.")

    platform = env["platform"]
    arch = env.get("arch", "")
    build_type = env.get("LIBPLATEAU_BUILD_TYPE", "Release")
    build_dir = get_libplateau_build_dir(platform, arch)

    cmake_args = [
        cmake_executable,
        "--build", str(build_dir),
        "--config", build_type,
    ]

    print(f"Building libplateau: {' '.join(cmake_args)}")
    subprocess.check_call(cmake_args)

    target_path = Path(str(target[0]))
    target_path.parent.mkdir(parents=True, exist_ok=True)
    target_path.touch()
    return 0


# Initialize environment
localEnv = Environment(tools=["default"], PLATFORM="")

customs = ["custom.py"]
customs = [os.path.abspath(path) for path in customs]

opts = Variables(customs, ARGUMENTS)
opts.Update(localEnv)

Help(opts.GenerateHelpText(localEnv))

env = localEnv.Clone()

# Ensure godot-cpp exists
if not (os.path.isdir("godot-cpp") and os.listdir("godot-cpp")):
    print_error("""godot-cpp is not available within this folder, as Git submodules haven't been initialized.
Run the following command to download godot-cpp:

    git submodule update --init --recursive""")
    sys.exit(1)

# Copy visionOS platform support to godot-cpp/tools before loading godot-cpp
visionos_src = REPO_ROOT / "patches" / "visionos.py"
visionos_dst = REPO_ROOT / "godot-cpp" / "tools" / "visionos.py"
if visionos_src.exists() and not visionos_dst.exists():
    print(f"Copying visionOS platform support: {visionos_src} -> {visionos_dst}")
    shutil.copy(str(visionos_src), str(visionos_dst))

# Load godot-cpp environment
env = SConscript("godot-cpp/SConstruct", {"env": env, "customs": customs})

# Get platform info
platform = env["platform"]
target = env["target"]
arch = env.get("arch", "")

# Ensure libplateau exists
if not LIBPLATEAU_ROOT.exists():
    print_error("""libplateau directory is missing. Please initialize the submodule:

    git submodule update --init --recursive""")
    sys.exit(1)

# Setup libplateau build
libplateau_build_type = get_cmake_build_type(target)
env["LIBPLATEAU_BUILD_TYPE"] = libplateau_build_type

libplateau_build_dir = get_libplateau_build_dir(platform, arch)
libplateau_lib_path = get_libplateau_lib_path(platform, libplateau_build_dir, libplateau_build_type)

# Check if we need to build libplateau
skip_libplateau_build = ARGUMENTS.get("skip_libplateau_build", "no") == "yes"

# Create File node for libplateau library
libplateau_lib_file = File(str(libplateau_lib_path))

if not skip_libplateau_build:
    # Configure step
    libplateau_configure = env.Command(
        str(libplateau_build_dir / ".cmake_configured"),
        [],
        configure_libplateau,
    )
    libplateau_configure_node = libplateau_configure[0]

    # Build step - target is the actual library file
    libplateau_build = env.Command(
        libplateau_lib_file,
        [libplateau_configure_node],
        build_libplateau,
    )
    libplateau_build_node = libplateau_build[0]
else:
    libplateau_build_node = None
    if not libplateau_lib_path.exists():
        print_error(f"""libplateau library not found at: {libplateau_lib_path}
Either build libplateau manually or run without skip_libplateau_build=yes""")
        sys.exit(1)

# Add include paths for libplateau
env.Append(CPPPATH=[
    str(LIBPLATEAU_ROOT / "include"),
    str(LIBPLATEAU_ROOT / "3rdparty" / "libcitygml" / "sources" / "include"),
    str(LIBPLATEAU_ROOT / "3rdparty" / "glm"),
])

# Link libplateau
env.Append(LIBS=[libplateau_lib_file])

# Platform-specific libraries
if platform == "windows":
    env.Append(LIBS=["glu32", "opengl32", "advapi32", "user32"])
elif platform == "linux":
    env.Append(LIBS=["GL", "GLU", "pthread", "dl"])
elif platform == "macos":
    env.Append(FRAMEWORKS=["OpenGL"])
elif platform == "android":
    env.Append(LIBS=["log", "android"])
elif platform == "ios":
    env.Append(FRAMEWORKS=["Foundation", "CoreGraphics"])
elif platform == "visionos":
    env.Append(FRAMEWORKS=["Foundation", "CoreGraphics"])

# Suppress warnings from libplateau headers and enable C++ exceptions
if platform == "windows":
    env.Append(CXXFLAGS=["/wd4251", "/wd4275", "/EHsc"])
else:
    env.Append(CXXFLAGS=["-Wno-deprecated-declarations"])
    if platform in ("android", "ios", "visionos"):
        env.Append(CXXFLAGS=["-fexceptions"])

# Source files
env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp") + Glob("src/plateau/*.cpp")

# Build suffix (remove .dev and .universal for compatibility)
suffix = env['suffix'].replace(".dev", "").replace(".universal", "")

lib_filename = "{}{}{}{}".format(env.subst('$SHLIBPREFIX'), LIB_NAME, suffix, env.subst('$SHLIBSUFFIX'))

# Determine output directory (include arch for mobile platforms)
if platform in ("android", "ios", "visionos"):
    bin_dir = "bin/{}/{}".format(platform, arch)
    addons_dir = "{}/addons/plateau/{}/{}/".format(GODOT_PROJECT_DIR, platform, arch)
else:
    bin_dir = "bin/{}".format(platform)
    addons_dir = "{}/addons/plateau/{}/".format(GODOT_PROJECT_DIR, platform)

# Build the library
library = env.SharedLibrary(
    "{}/{}".format(bin_dir, lib_filename),
    source=sources,
)

# Depend on libplateau build if we're building it
if libplateau_build_node:
    env.Depends(library, libplateau_build_node)

# Copy to demo project addons folder
copy_addons = env.Install(addons_dir, library)

default_args = [library] + copy_addons
Default(*default_args)
