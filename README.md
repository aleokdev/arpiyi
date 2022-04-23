# Important note
Arpiyi, as well as [Aryibi](https://github.com/alexdevteam/aryibi), are no longer in active development,
however, I might return to it at any point in time if
enough people are interested in it or I get inspired. Currently, the `master` branch is the most stable
one, `unstable` is kind of usable with a lot of newer features, and `new_tile_system` is in a very
heavy WIP. I was planning to merge `new_tile_system` into `unstable` and `unstable` into `master`,
but the amount of work needed to change everything is brutal. I made some really poor decisions while
starting the project (i.e. merging editor logic with GUI & layout or not thinking the map/sprite types properly),
leading into heavy tech debt down the line.

However, I've learned a lot with this project. Thanks to arpiyi, I learnt how to structure large projects,
how to use CMake properly, how to deal with dependencies correctly, how to use codegen in C++, basic
renderer technologies, many C++ features and quirks and much, much more. It's still one of my
favorite and largest projects.

## Arpiyi Editor
Arpiyi is an open-sourced 2D RPG editor designed to be modular and
extensible. Its objective is to be a more advanced alternative to other competing software,
with features such as a cutscene editor.

It is still on early development and lacks lots of features, but the project is updated
regularly with new elements.

### Current features
Arpiyi can:
- Load tilesets from images from various formats (Including PNG, JPEG, TGA, BMP...)
- Create maps for placing tiles
  - Maps don't have a fixed number or layer nor a layer limit:
  This means that you can reuse more tileset assets without having to redraw
  some of them for, for example, stacking them up
  - Layers can be also hidden, useful for working on one layer at a time
- Save & load projects

Arpiyi has:
- Autotiling
  - Supports RPGMaker VX/VX Ace/MV A2 textures
  - Automatically creates a texture with all possible corner & side combinations from a simpler image
- A Lua script editor
  - With a built-in syntax error checker

### Planned features
- Cutscene editor

### Requirements
A GPU with OpenGL 4.5 support. To check if your GPU is compatible, run the arpiyi executable.
You'll get an error if it isn't.

If your GPU does not have OpenGL 4.5 support, try updating your drivers.

### Building
**IMPORTANT: Clone to a path that doesn't have any spaces in it. Codegen has not been tested with spaced paths.**

Arpiyi compiles on Windows/Linux (Tested with clang-cl on Windows 10 & clang on Manjaro)

Either build the project directly to the `build/` folder or copy the `build/editor/data`
folder to your executable folder.

Linux requires the Lua 5.3 package for linking the executable. For Windows users, the library
is already included in the `lib/` folder, so you don't need to do anything special.

If any compiler errors arise while building, please open a Github issue for them.

### Contributing
For now, the best way to contribute is by testing, building and reporting issues.

If you'd like contribute by programming, here's a list of files that you should never touch
because of their extremely bad design and practices: (AKA TODO: clean those up)
- editor/editor_renderer.cpp (This file is probably going to be removed at some point. It contains some lua wrappers +
the main docking space of the application + the menu bar of it (The save button).)
