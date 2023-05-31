![license](https://img.shields.io/github/license/XLabsProject/s1x-client.svg)
[![open bugs](https://img.shields.io/github/issues/XLabsProject/s1x-client/bug?label=bugs)](https://github.com/XLabsProject/s1x-client/issues?q=is%3Aissue+is%3Aopen+label%3Abug)
[![Build](https://github.com/XLabsProject/s1x-client/workflows/Build/badge.svg)](https://github.com/XLabsProject/s1x-client/actions)
[![patreon](https://img.shields.io/badge/patreon-support-blue.svg?logo=patreon)](https://www.patreon.com/xlabsproject)
[![discord](https://img.shields.io/endpoint?url=https://momo5502.com/iw4x/members-badge.php)](https://discord.gg/sKeVmR3)


# S1x: Client

<p align="center">
  <img src="assets/github/banner.png?raw=true" />
</p>

<br/>

## Compile from source

- Clone the Git repo. Do NOT download it as ZIP, that won't work.
- Update the submodules and run `premake5 vs2022` or simply use the delivered `generate.bat`.
- Build via solution file in `build\s1x.sln`.

### Premake arguments

| Argument                    | Description                                    |
|:----------------------------|:-----------------------------------------------|
| `--copy-to=PATH`            | Optional, copy the EXE to a custom folder after build, define the path here if wanted. |
| `--dev-build`               | Enable development builds of the client. |

<br/>

## Disclaimer

This software has been created purely for the purposes of
academic research. It is not intended to be used to attack
other systems. Project maintainers are not responsible or
liable for misuse of the software. Use responsibly.
