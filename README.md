# [WispRenderer]() - Real-Time Raytracing Renderer

[![Release](https://img.shields.io/github/release/TeamWisp/WispRenderer.svg)](https://github.com/TeamWisp/WispRenderer/releases)
[![Issues](https://img.shields.io/github/issues/TeamWisp/WispRenderer.svg)](https://github.com/TeamWisp/WispRenderer/issues)
[![License](https://img.shields.io/badge/license-EPL%202.0-red.svg)](https://opensource.org/licenses/EPL-2.0)
[![Discord](https://img.shields.io/discord/486967125504688128.svg?color=blueviolet&label=Discord)](https://discord.gg/Q3vDfqR)
[![Twitter](https://img.shields.io/twitter/follow/wisprenderer.svg?style=social)](https://twitter.com/wisprenderer)
![](https://img.shields.io/github/stars/TeamWisp/WispRenderer.svg?style=social)

## [What is it?](https://teamwisp.github.io/product/)

Wisp is a general purpose high level rendering library. Specially made for real time raytracing with NVIDIA RTX graphics cards.

Suprted Rendering Backends:

* DirectX 12

Supported Platforms:

* Windows 10 (Version 1903)

Supported Compilers:

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

# Media
![Sponza](http://upload.vzout.com/wisp/sponza.png)

![Sponza](http://upload.vzout.com/wisp/sun_temple.jpg)
