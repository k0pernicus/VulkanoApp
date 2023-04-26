# Vulkano App

A template to build and deploy easily **Vulkan** applications, for Windows and macOS.

**This project is not compatible with Vulkan SDK > 1.3.211.**

This repository includes:

1. a simple application example,
2. a rendering engine.

## Compatibily

* macOS on Apple Silicon / Intel chips,
* Windows 10/11 with a dedicated graphics card and Vulkan enabled.

## About externs

This repository includes extern repositories / dependencies, like `imgui`, `vma`, or `glm`.
I tend to avoid git submodules as much as possible, so this repository contains everything to build and run the application.

The Vulkan engine uses `VMA` as memory allocator dependency. 

Thanks to the ImGui and GLM teams for providing such great tools for free.

## Build

Create an empty directory like `out/build` in the root of the project.

Now, build the Makefiles using cmake like this: `cmake -S ../../ -B . -DCMAKE_BUILD_TYPE=Debug -T ClangCL`.

I did not tried to use MSVC to compile the project but it should run (you may have to tweak the compiler's options...).

Once the Makefiles have been built, go in your `out/build` folder and launch make,
 or open your project in Visual Studio for Microsoft Windows.
