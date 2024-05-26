![license](https://img.shields.io/github/license/CBServers/s1x-client.svg)
[![build](https://img.shields.io/github/actions/workflow/status/CBServers/s1x-client/build.yml?branch=main&label=Build&logo=github)](https://github.com/CBServers/s1x-client/actions)
[![bugs](https://img.shields.io/github/issues/CBServers/s1x-client/bug?label=Bugs)](https://github.com/CBServers/s1x-client/issues?q=is%3Aissue+is%3Aopen+label%3Abug)
[![website](https://img.shields.io/badge/CBServers-Website-blue)](https://cbservers.xyz)


# S1x: Client
This is a fork of S1x/[s1-mod](https://git.alterware.dev/AlterWare/s1-mod).

Originally developed by [AlterWare](https://alterware.dev) and [X Labs](https://xlabs.dev/).

Thanks to all the original contributors.

<p align="center">
  <img src="assets/github/banner.png?raw=true" />
</p>

## Download

- **[Click here to get the latest release](https://github.com/CBServers/updater/raw/main/updater/s1x/s1x.exe)**
- **You will need to drop this in your Call of Duty: Advanced Warfare installation folder. If you don't have Call of Duty: Advanced Warfare, get those game files first.**
- The client is still in an early stage. It will have bugs!

## Compile from source

- Clone the Git repo. Do NOT download it as ZIP, that won't work.
- Update the submodules and run `premake5 vs2022` or simply use the delivered `generate.bat`.
- Build via solution file in `build\s1x.sln`.

### Premake arguments

| Argument                    | Description                                    |
|:----------------------------|:-----------------------------------------------|
| `--copy-to=PATH`            | Optional, copy the EXE to a custom folder after build, define the path here if wanted. |
| `--dev-build`               | Enable development builds of the client. |

## Disclaimer

This software has been created purely for the purposes of
academic research. It is not intended to be used to attack
other systems. Project maintainers are not responsible or
liable for misuse of the software. Use responsibly.
