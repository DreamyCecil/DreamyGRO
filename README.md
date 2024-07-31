# Dreamy GRO

*Time to say goodbye to the old-fashioned AutoGRO!*

**Dreamy GRO** is a custom utility designed to help mappers and modders pack their Serious Sam content into GRO packages without any hassle or extra bloat.

### Features
- One-click packaging of any world file with the automatic detection of the game directory.
- Ability to scan any file for the resources that they reference, including but not limited to: world files (`.wld`), game libraries (`.dll`), effect textures (`.tex`), message files (`.txt`).
- Mod support! For packaging files from mods using their standard resources.

### Supported games
- Serious Sam Classic: The First Encounter
- Serious Sam Classic: The Second Encounter
- Serious Sam Classics: Revolution *(works even better than RevPacker!)*
- *...and even other games on Serious Engine 1?!*

# Building

Before building the code, make sure to load in the submodules. Use `git submodule update --init --recursive` command to load files for all submodules.

The repository includes Visual Studio project files for building **Dreamy GRO** under Windows and Linux platforms (using WSL).

Windows project files are compatible with Visual Studio 2013 and higher, while Linux project files are compatible with Visual Studio 2019 and higher.

The solution includes projects for both platforms, so it's recommended to use Visual Studio 2019 or higher to build it.

### Tested compilers
- **MSVC**: v120, v142
- **GCC**: 9.4.0

# License

**Dreamy GRO** is licensed under the MIT license (see `LICENSE`).
