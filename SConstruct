#!/usr/bin/env python
"""
SConstruct for godot-plateau GDExtension
Build with: scons platform=<windows|linux|macos> target=<template_debug|template_release|editor>
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
def get_libplateau_build_dir(platform):
    """Get the libplateau build directory for the platform."""
    return BUILD_ROOT / platform / "libplateau"


def get_libplateau_lib_path(platform, build_dir, build_type):
    """Get the libplateau library path for the platform."""
    if platform == "windows":
        # Windows uses build type as subdirectory (Release/RelWithDebInfo/Debug)
        return build_dir / "src" / build_type / "plateau_combined.lib"
    elif platform == "macos":
        return build_dir / "src" / "libplateau.a"
    else:
        return build_dir / "src" / "libplateau_combined.a"


def get_cmake_build_type(target_name):
    """Map SCons target to CMake build type."""
    target_to_cmake = {
        "editor": "RelWithDebInfo",
        "template_debug": "RelWithDebInfo",
        "template_release": "Release",
    }
    return target_to_cmake.get(target_name, "Release")


def get_cmake_configure_args(platform, build_dir, build_type):
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
    else:  # linux
        return common_args


def configure_libplateau(target, source, env):
    """Configure libplateau with cmake."""
    cmake_executable = shutil.which("cmake")
    if not cmake_executable:
        raise RuntimeError("cmake executable not found in PATH.")

    platform = env["platform"]
    build_type = env.get("LIBPLATEAU_BUILD_TYPE", "Release")
    build_dir = get_libplateau_build_dir(platform)

    cmake_args = [
        cmake_executable,
        "-S", str(LIBPLATEAU_ROOT),
        "-B", str(build_dir),
    ] + get_cmake_configure_args(platform, build_dir, build_type)

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
    build_type = env.get("LIBPLATEAU_BUILD_TYPE", "Release")
    build_dir = get_libplateau_build_dir(platform)

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

# Load godot-cpp environment
env = SConscript("godot-cpp/SConstruct", {"env": env, "customs": customs})

# Get platform info
platform = env["platform"]
target = env["target"]

# Ensure libplateau exists
if not LIBPLATEAU_ROOT.exists():
    print_error("""libplateau directory is missing. Please initialize the submodule:

    git submodule update --init --recursive""")
    sys.exit(1)

# Setup libplateau build
libplateau_build_type = get_cmake_build_type(target)
env["LIBPLATEAU_BUILD_TYPE"] = libplateau_build_type

libplateau_build_dir = get_libplateau_build_dir(platform)
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

# Suppress warnings from libplateau headers and enable C++ exceptions
if platform == "windows":
    env.Append(CXXFLAGS=["/wd4251", "/wd4275", "/EHsc"])
else:
    env.Append(CXXFLAGS=["-Wno-deprecated-declarations"])

# Source files
env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp") + Glob("src/plateau/*.cpp")

# Build suffix (remove .dev and .universal for compatibility)
suffix = env['suffix'].replace(".dev", "").replace(".universal", "")

lib_filename = "{}{}{}{}".format(env.subst('$SHLIBPREFIX'), LIB_NAME, suffix, env.subst('$SHLIBSUFFIX'))

# Build the library
library = env.SharedLibrary(
    "bin/{}/{}".format(platform, lib_filename),
    source=sources,
)

# Depend on libplateau build if we're building it
if libplateau_build_node:
    env.Depends(library, libplateau_build_node)

# Copy to demo project addons folder
copy_addons = env.Install("{}/addons/plateau/{}/".format(GODOT_PROJECT_DIR, platform), library)

default_args = [library] + copy_addons
Default(*default_args)
