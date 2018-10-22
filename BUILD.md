# How to build Pepr3D

- [Building on Windows](#building-on-windows)
- [Building on Linux](#building-on-linux)

## Building on Windows

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

#### Windows

 1. Download and install CGAL https://www.cgal.org/download/windows.html

    Make sure CGAL_DIR environment variable is set (default when using the installer)

 2. Download and install Boost https://www.boost.org/

	Either build Boost yourself or download pre-built binaries for 14.1 toolset
    Make sure BOOST_ROOT environtment variable points to the boost directory.

#### Linux

 1. Install **CGAL**
 
	`sudo apt-get install libcgal-dev # install the CGAL library`
	
 2. Install **Boost**
 
	`sudo apt-get install libboost-all-dev`


#### FAQ:
- Do I need to install a whole Visual Studio IDE?
  
  You do **not** need a full Visual Studio IDE, it should be enough to download the [2017 build tools](https://visualstudio.microsoft.com/cs/downloads/). If you run on Windows 10, make sure to also check "Windows 10 SDK" during the installation process!

### 3. Building Pepr3D (including all dependencies)

Pepr3D can be built from a command line using the following 4 lines:

```cmd
> mkdir build
> cd build
build> cmake -G"Visual Studio 15 2017 Win64" ..
build> msbuild pepr3d.sln /m
```

#### Explanation

First of all, we use `cmake` to setup the build. The `cmake` command has to be run everytime you change `CMakeLists.txt`, which is a configuration file for CMake. From the root of the repo, execute the following:

```cmd
> mkdir build
> cd build
build> cmake -G"Visual Studio 15 2017 Win64" ..
```

CMake will generate necessary Visual Studio 2017 projects and a solution file `pepr3d.sln`, which you can either open directly in Visual Studio 2017 and build from the IDE, or you can build it from a command line.

#### Building from a command line

In order to build Pepr3D and all its dependencies from a command line, one has to be able to run `msbuild` of Visual Studio 2017. To use `msbuild`, either run **MSBuild Command Prompt for VS2017** or **Developer Command Prompt for VS 2017** from Start menu, or use the following command (may be different depending on your installation path of Visual Studio 2017):

```cmd
%comspec% /k "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat"
```

The following should appear in your command line:

```cmd
**********************************************************************
** Visual Studio 2017 Developer Command Prompt v15.7.1
** Copyright (c) 2017 Microsoft Corporation
**********************************************************************
```


Now that you have `msbuild` available, go to the root of the repo and execute:

```cmd
build> msbuild pepr3d.sln /m
```

### 4. Running Pepr3D

```cmd
build> .\pepr3d\Debug\pepr3d.exe
```

![image](https://user-images.githubusercontent.com/10374559/42907924-a17c08d0-8adf-11e8-8ba1-3b1af237d2a2.png)

## Building on Linux

We have a Linux [Docker container](https://www.docker.com/resources/what-container) set up at [tomasiser/docker-cinder](https://github.com/tomasiser/docker-cinder). Please read the following Dockerfiles to understand what is going on:

- [tomasiser/docker-cinder:latest Dockerfile](https://github.com/tomasiser/docker-cinder/blob/master/Dockerfile),
- [tomasiser/docker-cinder:prebuilt Dockerfile](https://github.com/tomasiser/docker-cinder/blob/prebuilt/Dockerfile),

where the `latest` image setups a Debian environment to build Cinder applications, and the `prebuilt` image actually builds Cinder on top of the `latest` image.

To build Pepr3D, it is enough to run `cmake` and `make` commands from this Pepr3D repository the usual way. You can get inspired by the [.circleci/config.yml](.circleci/config.yml) CI configuration file.
