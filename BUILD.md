# How to build Pepr3D

Here we explain how Pepr3D can be built from the source codes.
We assume some knowledge of build systems, compilers, and operating systems as this is a guide for developers.

## Building on Windows

We explain how the 64-bit Pepr3D can be built on Windows 8 and 10, which are the officially supported platforms.

### Repository

First of all, the official Pepr3D git repository has to be cloned.
This requires git to be installed on the machine and then cloning the repository using the following command in the Windows command line:

```
git clone --recurse-submodules -j8 https://github.com/tomasiser/pepr3d.git
```

If you have already accidentally cloned without submodules, run this command from the root directory of this repo:

```
git submodule update --init --recursive
```

### Dependencies

The following dependencies have to be downloaded and/or installed on the machine according to these steps:

* Download and install the latest **CMake** from https://cmake.org/.
* Download and install either **Visual Studio 2017** (Community version is enough) or alternatively only **Build Tools for Visual Studio 2017**. Both can be found at https://visualstudio.microsoft.com/downloads/.
* Download and install **CGAL** from https://www.cgal.org/download/windows.html. Make sure `CGAL_DIR` environment variable is set to the installed CGAL path, which is done by default when using the official installer.
* Download and install **Boost** from https://www.boost.org/. You can either build Boost yourself or download pre-built binaries for the 14.1 toolset. Make sure to point `BOOST_ROOT` environment variable to the installed Boost path.
* We use our own built version of **Assimp** from the latest `master` branch. Either build Assimp yourself from https://github.com/assimp/assimp, or download and unzip our prebuilt version (https://github.com/tomasiser/pepr3d/releases/download/v1.0/Assimp_for_Pepr3D.zip). Our version is built with two `.dll`, one for Debug and one for Release. Do not mix them up! Make sure to point `ASSIMP_ROOT` environment variable to the Assimp directory.
* Download **Freetype** from https://github.com/ubawurinna/freetype-windows-binaries, preferably version 2.9.1. After downloading, it is *necessary* to rename the `win64` subdirectory to `lib`. Make sure to point `FREETYPE_DIR` to the Freetype directory.

All other libraries are part of the Pepr3D repository and will be built automatically by our build system.

### Building

From the root directory of the cloned repository, run the following from the command line, which creates a new `build` directory and runs CMake inside:

```
mkdir build
cd build
cmake -G"Visual Studio 15 2017 Win64" ..
```

Now the build project is prepared inside the `build` subdirectory and we can now open `build/pepr3d.sln` in the Visual Studio 2017 application and compile Pepr3D from there.

#### Building from command line
Alternatively, we can build Pepr3D from the command line using the build tools. We have to start **MSBuild Command Prompt for VS2017** or **Developer Command Prompt for VS 2017** from Start Menu, or we can also start the command prompt from a standard command line using:

```
%comspec% /k "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat"
```

In the Visual Studio command prompt, we can build Pepr3D using:

```
msbuild pepr3d.sln /m
```

### Running

The executable of Pepr3D should be located in `build/pepr3d/Debug/pepr3d.exe` (or `Release` instead of `Debug`).

After building in Visual Studio, we have to make sure the appropriate `.dll` files are copied next to the executables.
If you used Visual Studio 2017 application to build it, some of the `.dll` files should be automatically copied to the executable directory.
If you used command line to build, you need to copy `libgmp-10.dll`, `libmpfr-4.dll`, and `assimp-vc140-mt.dll` manually from the `build/` directory to the same directory as `pepr3d.exe`.

#### Copying correct Assimp `.dll`
Note that by default, the Release version of Assimp `.dll` is copied.
If you built a Debug version of Pepr3D, you need to replace `assimp-vc140-mt.dll` by the file located in the directory where you unziped our Assimp library.
The Debug library is in the `bin/x64-Debug` subdirectory of Assimp instead of in `bin/x64`.
If you built Assimp on your own, you need to compile it in the same Debug or Release as Pepr3D.

#### Copying Freetype `.dll`
If you do not have `freetype.dll` as a part of your operating system already, you also need to copy this file next to the executable from the `lib` subdirectory of the Freetype you downloaded as described in the Dependencies subsection.

#### Running unit tests
By default, the Debug executable of all Pepr3D unit tests is build into `build/Debug/pepr3dtests.exe`.
It is necessary to also copy the `.dll` files there.

## Building on Linux / Docker container

There is a possibility to build Pepr3D on Linux systems, but please note that is in only supported for verifying that the source codes do compile as necessary for continuous integration.
It is not indended for running and using Pepr3D in release.

We have a Linux Docker container (https://www.docker.com/resources/what-container) in our special repository at GitHub: https://github.com/tomasiser/docker-cinder.
The `latest` image setups a Debian environment to build Cinder applications.
The `prebuilt` image actually builds Cinder on top of the `latest` image.
Pepr3D can be built in the `prebuilt` container by running `cmake` and `make` commands from the Pepr3D repository.

Note that in order to compile Pepr3D using the container, one needs to have at least a minimal experience with Docker.
We advise to follow the tutorials on the official Docker website (https://docs.docker.com/get-started/).