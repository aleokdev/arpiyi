## Arpiyi Editor
Arpiyi is an open-sourced 2D RPG editor in development designed to be modular,
extensible and most importantly, more advanced than competing software (i.e. RPGMaker).

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

### Planned features
- Lua script editor
  - With a built-in exception/warning system
- Cutscene editor

### Requirements
A GPU with OpenGL 4.5 support. To check if your GPU is compatible, run the arpiyi executable.
You'll get an error if it isn't.

If your GPU does not have OpenGL 4.5 support, try updating your drivers.

### Building
Arpiyi compiles on Windows/Linux (Tested with clang-cl on Windows 10 & clang on Manjaro)

Either build the project directly to the `build/` folder or copy the `build/editor/data`
folder to your executable folder.

Linux requires the Lua 5.3 package for linking the executable. For Windows users, the library
is already included in the `lib/` folder, so you don't need to do anything special.

If any compiler errors arise while building, please open a Github issue for them.