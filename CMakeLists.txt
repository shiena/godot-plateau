# Using the same minimum as the godot-cpp project
cmake_minimum_required(VERSION 3.17)

# Silence unused variable warning when specified from toolchain
if(CMAKE_C_COMPILER)
endif()

set(LIBNAME "godot-plateau" CACHE STRING "The name of the library")
set(GODOT_PROJECT_DIR "demo" CACHE STRING "The directory of a Godot project folder")

# libplateau settings - path to pre-built library (platform-specific defaults)
if(WIN32)
    set(LIBPLATEAU_BUILD_DIR_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/build/windows/libplateau")
    set(LIBPLATEAU_LIB_DEFAULT "${LIBPLATEAU_BUILD_DIR_DEFAULT}/src/RelWithDebInfo/plateau_combined.lib")
elseif(ANDROID)
    # Android arch mapping
    if(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        set(ANDROID_ARCH "arm64")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
        set(ANDROID_ARCH "arm32")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
        set(ANDROID_ARCH "x86_64")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
        set(ANDROID_ARCH "x86_32")
    else()
        set(ANDROID_ARCH "arm64")
    endif()
    set(LIBPLATEAU_BUILD_DIR_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/build/android/${ANDROID_ARCH}/libplateau")
    set(LIBPLATEAU_LIB_DEFAULT "${LIBPLATEAU_BUILD_DIR_DEFAULT}/src/libplateau.a")
elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(LIBPLATEAU_BUILD_DIR_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/build/ios/${CMAKE_OSX_ARCHITECTURES}/libplateau")
    # iOS builds as a framework
    set(LIBPLATEAU_LIB_DEFAULT "${LIBPLATEAU_BUILD_DIR_DEFAULT}/src/plateau.framework/plateau")
elseif(CMAKE_SYSTEM_NAME STREQUAL "visionOS" OR VISIONOS)
    set(LIBPLATEAU_BUILD_DIR_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/build/visionos/${CMAKE_OSX_ARCHITECTURES}/libplateau")
    # visionOS builds as a framework
    set(LIBPLATEAU_LIB_DEFAULT "${LIBPLATEAU_BUILD_DIR_DEFAULT}/src/plateau.framework/plateau")
elseif(APPLE)
    set(LIBPLATEAU_BUILD_DIR_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/build/macos/libplateau")
    set(LIBPLATEAU_LIB_DEFAULT "${LIBPLATEAU_BUILD_DIR_DEFAULT}/src/libplateau_combined.a")
else()
    set(LIBPLATEAU_BUILD_DIR_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/build/linux/libplateau")
    set(LIBPLATEAU_LIB_DEFAULT "${LIBPLATEAU_BUILD_DIR_DEFAULT}/src/libplateau_combined.a")
endif()

set(LIBPLATEAU_BUILD_DIR "${LIBPLATEAU_BUILD_DIR_DEFAULT}" CACHE PATH "Path to libplateau build directory")
set(LIBPLATEAU_COMBINED_LIB "${LIBPLATEAU_LIB_DEFAULT}" CACHE FILEPATH "Path to plateau_combined library")

# Make sure all the dependencies are satisfied
find_package(Python3 3.4 REQUIRED)
find_program(GIT git REQUIRED)

# Ensure godot-cpp submodule has been updated
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/godot-cpp/src")
    message(NOTICE "godot-cpp bindings source not found")
    message(NOTICE "initializing/updating the godot-cpp submodule...")

    execute_process(
        COMMAND git submodule update --init godot-cpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

# Copy visionOS platform support to godot-cpp/tools before loading godot-cpp
set(VISIONOS_PATCH_SRC "${CMAKE_CURRENT_LIST_DIR}/patches/visionos.py")
set(VISIONOS_PATCH_DST "${CMAKE_CURRENT_LIST_DIR}/godot-cpp/tools/visionos.py")
if(EXISTS "${VISIONOS_PATCH_SRC}" AND NOT EXISTS "${VISIONOS_PATCH_DST}")
    message(STATUS "Copying visionOS platform support: ${VISIONOS_PATCH_SRC} -> ${VISIONOS_PATCH_DST}")
    file(COPY "${VISIONOS_PATCH_SRC}" DESTINATION "${CMAKE_CURRENT_LIST_DIR}/godot-cpp/tools")
endif()

# Add godot-cpp subdirectory
add_subdirectory(godot-cpp SYSTEM)

# Add godot-cpp's module path and include the exported functions.
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${godot-cpp_SOURCE_DIR}/cmake")
include(GodotCPPModule)

# The godot-cpp target has some of useful properties attached
get_target_property(GODOTCPP_SUFFIX godot::cpp GODOTCPP_SUFFIX)
get_target_property(GODOTCPP_PLATFORM godot::cpp GODOTCPP_PLATFORM)

# Now we can specify our own project
project(godot-plateau
    VERSION 1.0
    DESCRIPTION "PLATEAU SDK for Godot - GDExtension for 3D city model data"
    HOMEPAGE_URL "https://github.com/shiena/godot-plateau"
    LANGUAGES CXX
)

add_library(${LIBNAME} SHARED)

target_sources(${LIBNAME}
    PRIVATE
    src/register_types.cpp
    src/register_types.h
    src/plateau/plateau_geo_reference.cpp
    src/plateau/plateau_geo_reference.h
    src/plateau/plateau_mesh_extract_options.cpp
    src/plateau/plateau_mesh_extract_options.h
    src/plateau/plateau_city_model.cpp
    src/plateau/plateau_city_model.h
    src/plateau/plateau_importer.cpp
    src/plateau/plateau_importer.h
    src/plateau/plateau_terrain.cpp
    src/plateau/plateau_terrain.h
    src/plateau/plateau_height_map_aligner.cpp
    src/plateau/plateau_height_map_aligner.h
    src/plateau/plateau_material_adjuster.cpp
    src/plateau/plateau_material_adjuster.h
    src/plateau/plateau_dataset_source.cpp
    src/plateau/plateau_dataset_source.h
    src/plateau/plateau_granularity_converter.cpp
    src/plateau/plateau_granularity_converter.h
    src/plateau/plateau_mesh_exporter.cpp
    src/plateau/plateau_mesh_exporter.h
    src/plateau/plateau_basemap.cpp
    src/plateau/plateau_basemap.h
)

# Fetch a list of the xml files to use for documentation and add to our target
file(GLOB_RECURSE DOC_XML LIST_DIRECTORIES NO CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/doc_classes/*.xml")

# conditionally add doc data to compile output
if(DOC_XML)
    if(GODOTCPP_TARGET MATCHES "editor|template_debug")
        target_doc_sources(${LIBNAME} ${DOC_XML})
    endif()
endif()

# Link with godot-cpp
target_link_libraries(${LIBNAME} PRIVATE godot-cpp)

# Check if pre-built libplateau exists and link
if(EXISTS "${LIBPLATEAU_COMBINED_LIB}")
    message(STATUS "Using pre-built libplateau: ${LIBPLATEAU_COMBINED_LIB}")
    target_link_libraries(${LIBNAME} PRIVATE "${LIBPLATEAU_COMBINED_LIB}")

    # Link system libraries required by libplateau
    if(WIN32)
        target_link_libraries(${LIBNAME} PRIVATE glu32 opengl32 advapi32 user32)
    elseif(ANDROID)
        target_link_libraries(${LIBNAME} PRIVATE log android)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        find_library(FOUNDATION_FRAMEWORK Foundation)
        find_library(COREGRAPHICS_FRAMEWORK CoreGraphics)
        target_link_libraries(${LIBNAME} PRIVATE ${FOUNDATION_FRAMEWORK} ${COREGRAPHICS_FRAMEWORK})
        # iOS needs 3rdparty libs separately (framework doesn't include them)
        target_link_libraries(${LIBNAME} PRIVATE
            "${LIBPLATEAU_BUILD_DIR}/3rdparty/libcitygml/lib/libcitygml.a"
            "${LIBPLATEAU_BUILD_DIR}/3rdparty/openmesh/src/OpenMesh/Core/libOpenMeshCore.a"
            "${LIBPLATEAU_BUILD_DIR}/3rdparty/openmesh/src/OpenMesh/Tools/libOpenMeshTools.a"
            "${LIBPLATEAU_BUILD_DIR}/3rdparty/hmm/src/libhmm.a"
            "${LIBPLATEAU_BUILD_DIR}/3rdparty/glTF-SDK/glTF-SDK/GLTFSDK/libGLTFSDK.a"
        )
    elseif(CMAKE_SYSTEM_NAME STREQUAL "visionOS" OR VISIONOS)
        find_library(FOUNDATION_FRAMEWORK Foundation)
        find_library(COREGRAPHICS_FRAMEWORK CoreGraphics)
        target_link_libraries(${LIBNAME} PRIVATE ${FOUNDATION_FRAMEWORK} ${COREGRAPHICS_FRAMEWORK})
        # visionOS needs 3rdparty libs separately (framework doesn't include them)
        target_link_libraries(${LIBNAME} PRIVATE
            "${LIBPLATEAU_BUILD_DIR}/3rdparty/libcitygml/lib/libcitygml.a"
            "${LIBPLATEAU_BUILD_DIR}/3rdparty/openmesh/src/OpenMesh/Core/libOpenMeshCore.a"
            "${LIBPLATEAU_BUILD_DIR}/3rdparty/openmesh/src/OpenMesh/Tools/libOpenMeshTools.a"
            "${LIBPLATEAU_BUILD_DIR}/3rdparty/hmm/src/libhmm.a"
            "${LIBPLATEAU_BUILD_DIR}/3rdparty/glTF-SDK/glTF-SDK/GLTFSDK/libGLTFSDK.a"
        )
    elseif(APPLE)
        find_library(OPENGL_FRAMEWORK OpenGL)
        find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
        find_library(CORESERVICES_FRAMEWORK CoreServices)
        target_link_libraries(${LIBNAME} PRIVATE
            ${OPENGL_FRAMEWORK}
            ${COREFOUNDATION_FRAMEWORK}
            ${CORESERVICES_FRAMEWORK}
        )
        # Find Homebrew prefix
        execute_process(
            COMMAND brew --prefix
            OUTPUT_VARIABLE HOMEBREW_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(NOT HOMEBREW_PREFIX)
            set(HOMEBREW_PREFIX "/opt/homebrew")
        endif()
        # Add mesa-glu for GLU functions (required by libcitygml)
        set(MESA_GLU_PREFIX "${HOMEBREW_PREFIX}/opt/mesa-glu")
        if(EXISTS "${MESA_GLU_PREFIX}")
            target_include_directories(${LIBNAME} PRIVATE "${MESA_GLU_PREFIX}/include")
            target_link_directories(${LIBNAME} PRIVATE "${MESA_GLU_PREFIX}/lib")
            target_link_libraries(${LIBNAME} PRIVATE GLU)
        endif()
        # Add liblzma and libdeflate for libtiff compression support
        target_link_directories(${LIBNAME} PRIVATE "${HOMEBREW_PREFIX}/lib")
        target_link_libraries(${LIBNAME} PRIVATE lzma deflate)
    else()
        # Linux
        find_package(OpenGL REQUIRED)
        target_link_libraries(${LIBNAME} PRIVATE OpenGL::GL OpenGL::GLU pthread dl)
    endif()
else()
    if(WIN32)
        message(FATAL_ERROR
            "Pre-built libplateau not found at: ${LIBPLATEAU_COMBINED_LIB}\n"
            "Please build libplateau first:\n"
            "  cmake -S libplateau -B build/windows/libplateau -G \"Visual Studio 17 2022\" -DPLATEAU_USE_FBX=OFF -DPLATEAU_USE_HTTP=OFF -DCMAKE_BUILD_TYPE:STRING=\"RelWithDebInfo\" -DBUILD_LIB_TYPE=static -DRUNTIME_LIB_TYPE=MT\n"
            "  cmake --build build/windows/libplateau --config RelWithDebInfo"
        )
    elseif(ANDROID)
        message(FATAL_ERROR
            "Pre-built libplateau not found at: ${LIBPLATEAU_COMBINED_LIB}\n"
            "Please build libplateau first:\n"
            "  cmake -S libplateau -B build/android/${ANDROID_ARCH}/libplateau -G \"Ninja\" -DPLATEAU_USE_FBX=OFF -DPLATEAU_USE_HTTP=OFF -DBUILD_LIB_TYPE=static -DCMAKE_BUILD_TYPE=Release -DANDROID_PLATFORM=android-24 -DANDROID_ABI=${CMAKE_ANDROID_ARCH_ABI} -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake\n"
            "  cmake --build build/android/${ANDROID_ARCH}/libplateau --config Release"
        )
    elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        message(FATAL_ERROR
            "Pre-built libplateau not found at: ${LIBPLATEAU_COMBINED_LIB}\n"
            "Please build libplateau first:\n"
            "  cmake -S libplateau -B build/ios/${CMAKE_OSX_ARCHITECTURES}/libplateau -G \"Ninja\" -DPLATEAU_USE_FBX=OFF -DPLATEAU_USE_HTTP=OFF -DBUILD_LIB_TYPE=static -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES} -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 -DCMAKE_C_FLAGS=\"-DPNG_ARM_NEON_OPT=0\"\n"
            "  cmake --build build/ios/${CMAKE_OSX_ARCHITECTURES}/libplateau --config Release"
        )
    elseif(CMAKE_SYSTEM_NAME STREQUAL "visionOS" OR VISIONOS)
        message(FATAL_ERROR
            "Pre-built libplateau not found at: ${LIBPLATEAU_COMBINED_LIB}\n"
            "Please build libplateau first:\n"
            "  cmake -S libplateau -B build/visionos/${CMAKE_OSX_ARCHITECTURES}/libplateau -G \"Ninja\" -DPLATEAU_USE_FBX=OFF -DPLATEAU_USE_HTTP=OFF -DBUILD_LIB_TYPE=static -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=godot-cpp/cmake/ios.toolchain.cmake -DPLATFORM=VISIONOS -DDEPLOYMENT_TARGET=1.0 -DIOS=ON\n"
            "  cmake --build build/visionos/${CMAKE_OSX_ARCHITECTURES}/libplateau --config Release"
        )
    elseif(APPLE)
        message(FATAL_ERROR
            "Pre-built libplateau not found at: ${LIBPLATEAU_COMBINED_LIB}\n"
            "Please build libplateau first:\n"
            "  cmake -S libplateau -B build/macos/libplateau -G \"Ninja\" -DPLATEAU_USE_FBX=OFF -DPLATEAU_USE_HTTP=OFF -DBUILD_LIB_TYPE=static -DRUNTIME_LIB_TYPE=MD -DCMAKE_BUILD_TYPE:STRING=\"Release\" -DCMAKE_CXX_FLAGS=\"-w\" -DCMAKE_OSX_ARCHITECTURES:STRING=\"arm64\"\n"
            "  cmake --build build/macos/libplateau --config Release"
        )
    else()
        message(FATAL_ERROR
            "Pre-built libplateau not found at: ${LIBPLATEAU_COMBINED_LIB}\n"
            "Please build libplateau first:\n"
            "  cmake -S libplateau -B build/linux/libplateau -DPLATEAU_USE_FBX=OFF -DPLATEAU_USE_HTTP=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_LIB_TYPE=static\n"
            "  cmake --build build/linux/libplateau --config RelWithDebInfo"
        )
    endif()
endif()

# Include directories for libplateau headers
target_include_directories(${LIBNAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/libplateau/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/libplateau/3rdparty/libcitygml/sources/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/libplateau/3rdparty/glm"
)

# Require at least C++17 for this target
set_property(TARGET ${LIBNAME} PROPERTY CXX_STANDARD 17)

# Suppress warnings from libplateau headers
if(MSVC)
    target_compile_options(${LIBNAME} PRIVATE /wd4251 /wd4275)
else()
    target_compile_options(${LIBNAME} PRIVATE -Wno-deprecated-declarations -Wno-switch -fexceptions)
endif()

set_target_properties(${LIBNAME}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "$<1:${PROJECT_SOURCE_DIR}/bin/${GODOTCPP_PLATFORM}>"
    PREFIX ""
    OUTPUT_NAME "${LIBNAME}${GODOTCPP_SUFFIX}"
)

set(GODOT_PROJECT_BINARY_DIR "${PROJECT_SOURCE_DIR}/${GODOT_PROJECT_DIR}/bin/${GODOTCPP_PLATFORM}")

add_custom_command(TARGET ${LIBNAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${LIBNAME}>" "${GODOT_PROJECT_BINARY_DIR}/$<TARGET_FILE_NAME:${LIBNAME}>"
)
