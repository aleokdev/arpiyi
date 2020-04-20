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
All data structures are contained in the `game` table.
#### Vec2
Stores a 2D point in space.

Pseudodefinition:
```
data Vec2 {
    /// Creates a new Vec2 from a X and Y component.
    new(float x, float y);

    float x,y { get; set; }
}
```
#### IVec2
Stores an 2D point in space composed by integers.

Pseudodefinition:
```
data IVec2 {
    /// Creates a new IVec2 from a X and Y component.
    new(int x, int y);

    int x,y { get; set; }
}
```
#### CameraClass
Defines the position, scale and effects of the camera.

There must always be a single instance of it, named `game.camera`.

Pseudodefinition:
```
data CameraClass {
    /// Position (Measured in tiles)
    Vec2 pos { get; set; }
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
    Vec2 pivot { get; set; }

    string name { get; }
    IVec2 size_in_pixels { get; }
};
```
#### Entity
Defines a dynamic object that may move and change state and has a sprite and scripts attached to it.

If the script is a child of an entity, it may access it via the readonly `entity` property.

Pseudodefinition:
```
data Entity {
    /// Position (Measured in tiles)
    Vec2 pos { get; set; }
    Sprite sprite { get; set; }
    string name { get; set; }
};
```
#### ScreenLayer
Defines an object that has a drawing callback and an order. Examples of these objects can be, for example, the map view,
an UI menu, etc.

This class contains static methods that you can call using `game.ScreenLayer.x()`, where `x` is the static function to
call.

Pseudodefinition:
```
data ScreenLayer {
    /// Creates a new ScreenLayer that is automatically added to the ScreenLayer list.
    /// ScreenLayers start visible and on the front.
    new(function render_callback);
    bool visible { get; set; }
    function render_callback { get; set; }
    
    /// Sends the screen layer to the top of the render queue, and thus is rendered last, after other layers.
    void to_front();
    /// Sends the screen layer to the top of the render queue, and thus is rendered last, in front of other layers.
    void to_back();

    /// Returns all the screenlayers created via new() or internally, which includes both hidden and visible ones.
    static ScreenLayer[] get_all();
    static ScreenLayer[] get_visible();
    static ScreenLayer[] get_hidden();
};
```

### Other definitions
#### Assets
You can use the `game.assets` function table to load resources that the game contains,
such as sprites, textures, etc.

Pseudodefinition:
```
table assets {
    /// Takes a resource path relative to the main game data / project path and returns the asset
    /// related to it, or nil if the path is not recognized or not associated with a type.
    any load(string path);
};
```
#### Input
The `game.input` table contains many functions and utils to get user input.

Pseudodefinition:
```
table input {
    /// An enumeration table containing all possible keys being held.
    table keys {
        k_A = 0,
        k_B = 1,
        k_C = 2,
        // ...
    };
    
    /// Contains the state of a key.
    data KeyState {
        /// True if the key is pressed, false otherwise.
        bool held();
        /// True if the key press had begun on the last frame.
        bool just_pressed();
        /// True if the key release had begun on the last frame.
        bool just_released();
    };

    /// Returns the key state of a particular key from the keys table.
    KeyState get_key_state(int);
};
```