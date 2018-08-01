# How to build Pepr3D

## Building on Windows from scratch

### 1. Cloning the repo with submodules

First of all, make sure you clone this repository including submodules, e.g., including `lib/cinder`. This is done like this:

```
git clone --recurse-submodules -j8 https://github.com/tomasiser/pepr3d.git
```

If you have already accidentally cloned without submodules, run this command from the root directory of this repo:

```
git submodule update --init --recursive
```

### 2. Installing dependencies

Make sure you have the latest **CMake** and **build tools for Visual Studio 2017** installed.

#### FAQ:
- Do I need to install a whole Visual Studio IDE?
  
  You do **not** need a full Visual Studio IDE, it should be enough to download the [2017 build tools](https://visualstudio.microsoft.com/cs/downloads/). If you run on Windows 10, make sure to also check "Windows 10 SDK" during the installation process!

### 3. Building Pepr3D (including all dependencies)

In order to build Pepr3D and all its dependencies, run **MSBuild Command Prompt for VS2017** or **Developer Command Prompt for VS 2017** in start menu (a command prompt bundled in Visual Studio 2017 that can run `msbuild`), go to the root of the repo, and execute:

```cmd
> mkdir build
> cd build
build> cmake -G"Visual Studio 15 2017 Win64" ..
build> msbuild pepr3d.sln /m
```

### 4. Running Pepr3D

```cmd
build> .\pepr3d\Debug\pepr3d.exe
```

![image](https://user-images.githubusercontent.com/10374559/42907924-a17c08d0-8adf-11e8-8ba1-3b1af237d2a2.png)
