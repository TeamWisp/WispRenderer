# [<img src="http://upload.vzout.com/wisp/logo.png" width="40"> WispRenderer](https://teamwisp.github.io) - Real-Time Raytracing Renderer

[![Release](https://img.shields.io/github/release/TeamWisp/WispRenderer.svg)](https://github.com/TeamWisp/WispRenderer/releases)
[![Issues](https://img.shields.io/github/issues/TeamWisp/WispRenderer.svg)](https://github.com/TeamWisp/WispRenderer/issues)
[![License](https://img.shields.io/badge/license-EPL%202.0-red.svg)](https://opensource.org/licenses/EPL-2.0)
[![Discord](https://img.shields.io/discord/486967125504688128.svg?color=blueviolet&label=Discord)](https://discord.gg/Q3vDfqR)
[![Twitter](https://img.shields.io/twitter/follow/wisprenderer.svg?style=social)](https://twitter.com/wisprenderer)
![](https://img.shields.io/github/stars/TeamWisp/WispRenderer.svg?style=social)

## [What is it?](https://teamwisp.github.io/product/)

Wisp is a general purpose high level rendering library. Specially made for real time raytracing with NVIDIA RTX graphics cards. Made by a group of students from [Breda University of Applied Sciences](https://www.buas.nl/) specializing in graphics programming.

**Features**

* Physically Based Rendering
* Ray Traced Global Illumination
* Path Traced Global Illumination
* Ray Traced Ambient Occlusion
* Ray Traced Reflections
* Ray Traced Shadows
* Translucency & Transparency.
* NVIDIA's HBAO+
* NVIDIA's AnselSDK

**Supported Rendering Backends**

* DirectX 12

**Supported Platforms**

* Windows 10 (Version 1903)

**Supported Compilers**

* Visual Studio 2017
* Visual Studio 2019


## [Installation](https://teamwisp.github.io/workspace_setup/)

### Installer

```
git clone https://github.com/TeamWisp/WispRenderer.git
```

Then run `installer.exe` located in the new `WispRenderer` folder and follow the on-screen prompts. When finished you can find the `.sln` file in the `build_vs2019_win64` folder.


### Manual

```
git clone https://github.com/TeamWisp/WispRenderer.git
cd WispRenderer
mkdir build
cd build
cmake -G "Visual Studio 16 2019" ..
```

Now you can run the `.sln` file inside the `build` folder. Want to use NVIDIA Gameworks? See the [advanced documation](https://teamwisp.github.io/workspace_setup/).

# [Documentation](https://teamwisp.github.io/)

# [Example](https://github.com/TeamWisp/WispRenderer/tree/master/demo)

# [Getting Involved](https://teamwisp.github.io/)

Want to help us out? That's definatly possible! Check out our [contribution page](https://teamwisp.github.io/) on how.

# [Development Blog](https://teamwisp.github.io/WispBlog/)

# [Discord](https://discord.gg/Q3vDfqR)

Need help, want to get updates as soon as they happen or just want a chat? Join our [Discord Server](https://discord.gg/Q3vDfqR)!

## Trailer

<a href="http://www.youtube.com/watch?feature=player_embedded&v=JsqF1jyyz2M" 
target="_blank"><img src="http://img.youtube.com/vi/JsqF1jyyz2M/0.jpg" 
alt="Torque 6 Material Editor" border="0" /></a>

## Media

<img src="http://upload.vzout.com/wisp/sponza.png" width="300"> <img src="http://upload.vzout.com/wisp/sun_temple.jpg" width="300">

## [License (Eclipse Public License version 2.0)](https://opensource.org/licenses/EPL-2.0)

<a href="https://opensource.org/licenses/EPL-2.0" target="_blank">
<img align="right" src="http://opensource.org/trademarks/opensource/OSI-Approved-License-100x137.png">
</a>

```
Copyright 2018-2019 Breda University of Applied Sciences

If a Contributor Distributes the Program in any form, then:
   a) the Program must also be made available as Source Code, in accordance with section 3.2, and the Contributor must accompany the Program with a statement that the Source Code for the Program is available under this Agreement, and informs Recipients how to obtain it in a reasonable manner on or through a medium customarily used for software exchange; and
   b) the Contributor may Distribute the Program under a license different than this Agreement, provided that such license:
      i) effectively disclaims on behalf of all other Contributors all warranties and conditions, express and implied, including warranties or conditions of title and non-infringement, and implied warranties or conditions of merchantability and fitness for a particular purpose;
      ii) effectively excludes on behalf of all other Contributors all liability for damages, including direct, indirect, special, incidental and consequential damages, such as lost profits;
      iii) does not attempt to limit or alter the recipients' rights in the Source Code under section 3.2; and
      iv) requires any subsequent distribution of the Program by any party to be under a license that satisfies the requirements of this section 3.

When the Program is Distributed as Source Code:
   a) it must be made available under this Agreement, or if the Program (i) is combined with other material in a separate file or files made available under a Secondary License, and (ii) the initial Contributor attached to the Source Code the notice described in Exhibit A of this Agreement, then the Program may be made available under the terms of such Secondary Licenses, and
   b) a copy of this Agreement must be included with each copy of the Program.

Contributors may not remove or alter any copyright, patent, trademark, attribution notices, disclaimers of warranty, or limitations of liability (‘notices’) contained within the Program from any copy of the Program which they Distribute, provided that Contributors may add their own appropriate notices.
```
