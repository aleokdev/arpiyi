## Lua Public API

### Scripts
Scripts modify an entity's operation. Automatically, they are run depending on their auto-trigger:
- None: The script will not be run automatically (Hence something else needs to trigger it, like the player)
- Autorun: The script will be run once at load.

Here's a sample script that makes the camera follow the parent entity indefinitely:
```lua
while true do
    camera.pos = entity.pos
end
```

### Data Structures
#### Fvec2
Stores a floating-point 2D point in space.

Pseudodefinition:
```
data fvec2 {
    /// Creates a new fvec2 from a X and Y component.
    fvec2 new(float x, float y);

    float x,y { get; set; }
}
```
#### CameraClass
Defines the position, scale and effects of the camera.

There must always be a single instance of it, named `Camera`.

Pseudodefinition:
```
data CameraClass {
    /// Position (Measured in tiles)
    fvec2 pos { get; set; }
    /// Camera zoom, measured in screen pixels per asset pixels. (i.e. 2 => 200% zoom.)
    float zoom { get; set; }
} camera;
```
#### Sprite
Defines a textured and named asset that contains a pivot. Used in entities.

Pseudodefinition:
```
data Sprite {
    /// Pivot of the sprite, aka where it scales/rotates/translates from.
    /// {0,0} means upper left corner of the image, {1,1} means lower right.
    fvec2 pivot { get; set; }
};
```
#### Entity
Defines a dynamic object that may move and change state and has a sprite and scripts attached to it.

If the script is a child of an entity, it may access it via the readonly `entity` property.

Pseudodefinition:
```
data Entity {
    /// Position (Measured in tiles)
    fvec2 pos { get; set; }
    Sprite sprite { get; set; }
};
```