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

Make sure you have the latest **CMake** and **build tools for Visual Studio 2015** installed.

#### FAQ:
- Do I need to install a whole Visual Studio IDE?
  
  You do **not** need a full Visual Studio IDE, it should be enough to download the [2015 build tools](https://blogs.msdn.microsoft.com/vcblog/2015/11/02/announcing-visual-c-build-tools-2015-standalone-c-tools-for-build-environments/) (not 2017). If you run on Windows 10, make sure to also check "Windows 10 SDK" during the installation process!
  
- What if I have Visual Studio 2017?

  You can use the IDE, but you still need build tools for 2015. When you load the project in Visual Studio 2017, make sure to **not upgrade** the solution to 2017!
  
  ![image](https://user-images.githubusercontent.com/10374559/42907132-3bc3915e-8add-11e8-933d-7ffbbbfaf56e.png)
  ![image](https://user-images.githubusercontent.com/10374559/42907179-56236218-8add-11e8-88fe-d81be148cfd4.png)
  
### 3. Building Cinder

> :information_source: **You can skip this step** if you directly [download prebuilt Cinder from 2018-07-18](https://1drv.ms/u/s!AuC8m2CvOUXjgdlZthWalFAZjQ0fkw) (~745MB .zip) and unzip the archive into the root directory of this repo, i.e., the `lib/cinder` inside the .zip should match `lib/cinder` inside the repo (overwrite all files). **Pay attention to `git`** if you do this: make sure not to commit anything inside the `cinder` submodule!

In order to build Cinder, run **MSBuild Command Prompt for VS2015** (a command prompt bundled in Visual Studio 2015 that can run `msbuild`), go to `lib/cinder`, and execute:

```cmd
lib\cinder> mkdir build
lib\cinder> cd build
lib\cinder\build> cmake -DCMAKE_GENERATOR_PLATFORM=x64 ..
lib\cinder\build> msbuild cinder.sln /m
```

### 4. Building Pepr3D

Again, use **MSBuild Command Prompt for VS2015**, go to the root of this repo, and execute:

```cmd
> mkdir build
> cd build
build> cmake -DCMAKE_GENERATOR_PLATFORM=x64 ..
build> msbuild pepr3d.sln /m
```

### 5. Running Pepr3D

```cmd
build> .\pepr3d\Debug\pepr3d.exe
```

![image](https://user-images.githubusercontent.com/10374559/42907924-a17c08d0-8adf-11e8-8ba1-3b1af237d2a2.png)
