# tree-gen

Fork of [QuTech-Delft/tree-gen](https://github.com/qutech-delft/tree-gen), a QuTech/TU Delft C++ and Python code generator for tree-like structures.

This fork is playing with:
- Conan package manager.
- C++23.
- CMake 3.20.

## Build

```
~/projects/tree-gen> conan install . -b=missing
~/projects/tree-gen> conan create .
~/projects/tree-gen> conan upload tree-gen/0.1 -r=rturradocenter
```
