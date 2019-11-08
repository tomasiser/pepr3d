<h1 align="center"><img height="131" alt="Pepr3D" src="https://user-images.githubusercontent.com/10374559/61998646-4c76cf00-b0b3-11e9-8e30-f88d70435b37.png"></h1>

<h3 align="center">
  <a href="https://github.com/tomasiser/pepr3d/releases">Downloads</a>
  &bull;
  <a href="https://github.com/tomasiser/pepr3d/releases/download/v1.0/Pepr3D-Windows-x64.zip">Windows 64-bit</a>
  &bull;
  <a href="https://github.com/tomasiser/pepr3d/releases/download/v1.0/Pepr3D-Documentation.pdf">Documentation</a>
  &bull;
  <a href="https://github.com/tomasiser/pepr3d/wiki">Wiki</a>
</h3>

<h4 align="center">
  <a href="BUILD.md">How to build</a>
  &bull;
  <a href="https://tomasiser.github.io/pepr3d/">Generated documentation</a>
</h4>

<h1></h1>

<p align="center">
<img alt="Build Badge" src="https://img.shields.io/github/languages/top/tomasiser/pepr3d" />
<img alt="Build Badge" src="https://img.shields.io/circleci/build/github/tomasiser/pepr3d" />
<img alt="Download Badge" src="https://img.shields.io/github/downloads/tomasiser/pepr3d/latest/total?color=amber&label=downloads" />
</p>

<h1></h1>

<h3 align="center"><img height="400" alt="Pepr3D Screenshot" src="https://user-images.githubusercontent.com/10374559/62304598-94fd0680-b47e-11e9-8b5d-cc1fe54af8f8.png"></h3>

## Key Features

**Pepr3D** is an intuitive, free, and [open-source](LICENSE.md) 3D painting tool for coloring models intended for colorful 3D printing.

- Primarily, we support **multi-material 3D FDM printers** such as [Prusa printers](https://www.prusa3d.com/) with their [multi-material upgrades](https://www.prusa3d.com/original-prusa-i3-multi-material-2-0/).

- **Import** existing single-color 3D models in various formats (`.obj`, `.stl`, and more), e.g., from [Thingiverse](https://www.thingiverse.com/).

- **Color** the model surface with intuitive tools such as *triangle painter* or *paint bucket* that follow the triangle topology.

- **Paint** details and texts using *brush* and *text editor* tools. They are **not limited by the original topology!**

- For faster painting, **segment** the model automatically or semi-automatically.

- **Export** the final result into individual `.stl` files using our easy-to-use export assistant.

- The exported files can be **printed** directly via the official [Prusa Slicer](https://www.prusa3d.com/prusaslicer/) ([GitHub](https://github.com/prusa3d/PrusaSlicer)) using a [multi-material Prusa 3D printer](https://www.prusa3d.com/original-prusa-i3-multi-material-2-0/).

## Examples

| Model import | Triangle painter |
|:-------------------------:|:-------------------------:|
|<img alt="Model import" src="https://user-images.githubusercontent.com/10374559/62309816-7c91e980-b488-11e9-87e4-37ad81093d32.gif">|<img alt="Triangle painter" src="https://user-images.githubusercontent.com/10374559/62309982-d5618200-b488-11e9-9f1a-06698eca3bf8.gif">|

| Paint bucket | Brush |
|:-------------------------:|:-------------------------:|
|<img alt="Paint bucket" src="https://user-images.githubusercontent.com/10374559/62310156-2e311a80-b489-11e9-97a0-fc6bb29b8a5f.gif">|<img alt="Brush" src="https://user-images.githubusercontent.com/10374559/62310365-a7c90880-b489-11e9-93cf-ea9d1f6d6093.gif">|


| Text editor | Export assistant |
|:-------------------------:|:-------------------------:|
|<img alt="Text editor" src="https://user-images.githubusercontent.com/10374559/62310753-7f8dd980-b48a-11e9-966c-46a670986ed5.gif">|<img alt="Export assistant" src="https://user-images.githubusercontent.com/10374559/62311235-af89ac80-b48b-11e9-8b79-e33f6190ced8.gif">|

## Printing results

![Printing results](https://user-images.githubusercontent.com/10374559/62311508-42c2e200-b48c-11e9-96fa-c481779d1152.png)

## Authors

Pepr3D was originally created as a university project at the [Faculty of Mathematics and Physics](https://www.mff.cuni.cz/en), [Charles University](https://cuni.cz/UKEN-1.html) within the [Computer Graphics Group](https://cgg.mff.cuni.cz/index.en.php) with the cooperation of [Prusa Research](https://www.prusa3d.com/).

### Main contributors (alphabetically)
* Štěpán Hojdar
* Tomáš Iser
* Jindřich Pikora
* Luis Sanchez

### Supervisor
* [Oskár Elek](https://cgg.mff.cuni.cz/~oskar/), [CGG @ MFF CUNI](https://cgg.mff.cuni.cz/index.en.php), now [Creative Coding @ UC Santa Cruz](https://creativecoding.soe.ucsc.edu/people.php)

### Consultants (alphabetically)
* Vojtěch Bubník, [Prusa Research](https://www.prusa3d.com/)
* [Jaroslav Křivánek](https://cgg.mff.cuni.cz/~jaroslav/), [CGG @ MFF CUNI](https://cgg.mff.cuni.cz/index.en.php)
* [Tobias Rittig](https://cgg.mff.cuni.cz/~tobias/), [CGG @ MFF CUNI](https://cgg.mff.cuni.cz/index.en.php)

*The ["Bulbasaur" model](https://www.thingiverse.com/thing:327753) in the screenshots and photos by [Agustin Flowalistik](https://www.thingiverse.com/FLOWALISTIK/about). More sample models can be [downloaded](https://github.com/tomasiser/pepr3d/releases/download/v1.0/Pepr3D-SampleModels.zip) from the [Releases](https://github.com/tomasiser/pepr3d/releases) page. Third-party libraries used in the project are documented in the [LICENSE.md](LICENSE.md) file.*
