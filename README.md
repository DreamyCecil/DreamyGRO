# Dreamy GRO

*Time to say goodbye to old-fashioned AutoGRO!*

**Dreamy GRO** is a custom utility designed to help mappers automatically pack their Serious Sam levels into GRO archives with all the needed resources that the levels may use.

### Supported games
- Serious Sam Classic: The First Encounter
- Serious Sam Classic: The Second Encounter
- Serious Sam Classics: Revolution *(works even better than RevPacker!)*
- *...and even other games on Serious Engine 1?!*

## Building

Before building the code, make sure to load in the submodules. Use `git submodule update --init --recursive` command to load files for all submodules.

The repository includes Visual Studio project files for building **Dreamy GRO** under Windows and Linux platforms (using WSL).

Windows project files are compatible with Visual Studio 2013 and higher, while Linux project files are compatible with Visual Studio 2019 and higher.

The solution includes projects for both platforms, so it's recommended to use Visual Studio 2019 or higher to build it.

### Tested compilers
- **MSVC:** 12.0, 14.29
- **GCC:** 9.4.0

## License

**Dreamy GRO** is licensed under the MIT license (see `LICENSE`).
