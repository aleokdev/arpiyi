## Controls
This will be replaced by a tutorial later on.

LMB = Left Mouse Button.

RMB = Right Mouse Button.

MMB = Middle Mouse Button.

### Map view
You can move the camera by holding the MMB.
![Toolbar](https://i.imgur.com/ySB77Nd.png)
- First icon: **Terrain editing tool**. Use this for placing tiles.
- Second icon: **Comment editing tool**. Broken on current build.
- Third icon: **Entity editing tool**. Use this to place, edit and move scriptable
entities.
- Fourth icon: **Height editing tool**. Use this to create shadows by changing the height
of certain parts of your map.

#### Terrain editing tool
LMB: Action depends on tileset:
- If the tileset is non-automatic, LMB places the current selected tile(s) directly to
below the cursor, as shown in the preview.
- If the tileset is automatic, controls are a little bit more complex: (An image is
added for clarity. The map contained 9 sand tiles and was clicked on the center with the
corresponding option)
    ![LMB](https://i.imgur.com/QtiVcZI.png)
    - LMB will place the type of tile selected and adjust itself and its surroundings
    so that the textures matches perfectly.
    ![LMB + Shift](https://i.imgur.com/nd6x0r6.png)
    - LMB + Shift (_Quiet mode_) will make the tile adjust its own texture and its neighbours' **only
    if they're the same type as the one you're placing.**
    ![LMB + Ctrl](https://i.imgur.com/VcIn9TU.png)
    - LMB + Ctrl (_Join mode_) will make the tile adjust its own texture and will join its neighbours
    into tiles of the same type as the selection, even if they're of a different type.

#### Entity editing tool
Double click LMB to create a new entity. Drag and select entities with LMB.

#### Height editing tool
LMB Click: Raise the terrain by one unit.
RMB Click: Lower the terrain by one unit.
Shift + LMB Click: Cycle through slope type. (This is broken on maps with automatic
tilesets.)
Ctrl + RMB Click: Toggles _floating mode_. If a tile has this mode enabled, it won't
generate side wall shadows.

![Non-floating shadows](https://i.imgur.com/HGuJHTg.png)
![Floating shadows](https://i.imgur.com/a4BAJGV.png)