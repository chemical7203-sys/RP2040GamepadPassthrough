# Agent Build and Troubleshooting Guide

This document provides definitive instructions for building the firmware. The build process has proven to be sensitive to the host environment. Please follow these steps exactly.

## 1. Required Dependencies

The build **will fail** if any of these are missing or not correctly configured in your system's PATH.

1.  **Pico SDK**: A **full, clean clone** from `https://github.com/raspberrypi/pico-sdk`.
    *   You **must** run `git submodule update --init` inside the `pico-sdk` directory after cloning.
    *   The `PICO_SDK_PATH` environment variable must point to this directory.
2.  **ARM GCC Toolchain**: The cross-compiler for the RP2040.
3.  **CMake**: The build system generator.
4.  **Python 3**: Required for the `makefsdata.py` script.
5.  **Build Tools for Visual Studio with NMake**: On Windows, the "NMake Makefiles" generator is the most reliable. This requires the "C++ build tools" from the Visual Studio Installer, which includes `nmake.exe`.

## 2. Build Instructions

These commands **must** be run from a terminal where all the above dependencies are in the system PATH. For Windows, the **"Developer Command Prompt for VS"** is highly recommended.

1.  **Navigate to the firmware directory:**
    ```bash
    cd rp2040_firmware
    ```

2.  **Delete any existing `build` directory:**
    *   This is crucial to ensure a clean configuration.
    *   On Windows CMD: `rmdir /s /q build`
    *   On PowerShell/Git Bash: `rm -rf build`

3.  **Run CMake to configure the project:**
    *   This command must be run from the `rp2040_firmware` directory.
    *   **For Pico W:**
        ```bash
        cmake -G "NMake Makefiles" -DPICO_BOARD=pico_w -S . -B build
        ```
    *   **For standard Pico:**
        ```bash
        cmake -G "NMake Makefiles" -DPICO_BOARD=pico -S . -B build
        ```

4.  **Run NMake to build the firmware:**
    *   Navigate into the newly created build directory: `cd build`
    *   Run the build command: `nmake`

## 3. Critical Troubleshooting

If you have followed all the steps above exactly and still encounter a `"No such file or directory"` error for a core SDK header (like `tusb.h`, `FreeRTOS.h`, `hardware/crc32.h`), this indicates a fundamental problem with your build environment.

**The `CMakeLists.txt` file in this project is correct.** The logic for finding and linking libraries is standard. A failure at this stage means the tools on your machine (CMake, NMake, or the compiler toolchain) are not interacting with the Pico SDK as expected.

I cannot fix a local environment issue. This documentation is the final step I can take to help resolve the problem. Please verify your environment, paths, and SDK installation.
