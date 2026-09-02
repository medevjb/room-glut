## 1. Skeleton and build

- [x] 1.1 Create `main.cpp` with the platform-guarded GLUT/GL/GLU includes (`__APPLE__` → `<GLUT/glut.h>`, else `<GL/glut.h>`) and a header comment carrying the three build commands (macOS: `clang++ main.cpp -o room -framework OpenGL -framework GLUT -Wno-deprecated-declarations`; Linux: `g++ main.cpp -o room -lGL -lGLU -lglut`; Windows: freeglut/MSVC). Verify: file compiles to an empty program on macOS with no errors.
- [x] 1.2 Add `main()` with `glutInit`, `GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH`, a 1200×800 titled window, and `glutMainLoop()`. Verify: running the binary opens a blank titled window that closes cleanly.
- [x] 1.3 Add `initGL()` enabling `GL_DEPTH_TEST`, `GL_LIGHTING`, `GL_NORMALIZE`, `GL_COLOR_MATERIAL` off, smooth shading, and a dark clear colour; register `display`, `reshape`, `keyboard`, `passiveMotion`, and `timer` callbacks. Verify: window renders a cleared dark frame and callbacks fire (temporary printf or no-op).
- [x] 1.4 Implement `reshape(w,h)` with `gluPerspective(60, w/h, 0.1, 100)` guarding h==0. Verify: resizing the window keeps a drawn test cube square, not stretched (camera-navigation: "Resize keeps proportions").

## 2. Geometry and material primitives

- [x] 2.1 Add `struct Vec3` with add/scale/cross/normalize helpers used by the camera. Verify: a scratch assertion or printf shows `normalize` returns unit length and `cross` is perpendicular.
- [x] 2.2 Implement `drawBox(w,h,d)` emitting 6 quads with an explicit `glNormal3f` per face, centred on the local origin. Verify: with one light on, the six faces of a test box shade differently and none renders black (lighting-and-materials: "Lighting varies across orientations", "No inside-out shading").
- [x] 2.3 Implement `drawPlaneGrid(w,d,n,normal)` subdividing a plane into n×n quads with the given normal, for floor/walls/ceiling. Verify: a spot light aimed at the grid produces a visible round pool of light rather than uniform flat shading.
- [x] 2.4 Add `struct Material` and `setMaterial()` plus the named constants `MAT_WOOD`, `MAT_FABRIC`, `MAT_METAL`, `MAT_GLASS`, `MAT_WALL`, `MAT_FLOOR`, `MAT_RUG`. Verify: drawing a metal box directly after a fabric box shows a tight highlight on the metal and none on the fabric (lighting-and-materials: "Materials are visually distinct", "No material bleed").

## 3. Room shell and layout constants

- [x] 3.1 Add the commented world-layout constants block (room 12×3.2×9 centred on origin, floor y=0, plus every object anchor and facing angle from design.md D2). Verify: constants compile and each is referenced by name, not duplicated as a literal at the call site.
- [x] 3.2 Implement `drawRoom()` drawing floor, ceiling, and four walls with `drawPlaneGrid`, normals pointing into the room, using `MAT_FLOOR`/`MAT_WALL`. Verify: from inside, all six surfaces are visible and lit with no gaps (room-scene: "Room is fully enclosed", "Interior-facing normals").

## 4. Camera navigation

- [x] 4.1 Add camera state (`camPos`, `yaw`, `pitch`, `moveSpeed`, `mouseSens`) and `applyCamera()` computing forward/right and calling `gluLookAt(camPos, camPos+forward, up)` at the top of `display`. Verify: the starting view is inside the room looking at the living area.
- [x] 4.2 Implement WASD/QE translation in `keyboard()` (case-insensitive) using the forward/right vectors and world +Y. Verify: pressing `W` while pitched moves along the look direction, `A`/`D` strafe without rotating, `Q`/`E` move straight up/down (camera-navigation scenarios).
- [x] 4.3 Implement `passiveMotion()` mouse-look: delta from window centre, yaw/pitch update, pitch clamped to ±89°, `glutWarpPointer` recentre with an `ignoreNextMotion` re-entrancy guard. Verify: moving the mouse turns the view smoothly with no jitter, and pushing up/down continuously stops at vertical without flipping.
- [x] 4.4 Bind arrow keys as a fallback look control via `glutSpecialFunc`. Verify: arrow keys yaw/pitch the same way as the mouse.

## 5. Lighting

- [x] 5.1 Configure `GL_LIGHT0` (positional, at the chandelier, white, ambient+diffuse) and `GL_LIGHT1` (warm spot, cutoff ~35°, exponent ~12, attenuation) in `initLights()`, and re-specify both positions **after** `gluLookAt` each frame. Verify: moving the camera does not move the lit/shadowed sides of objects (lighting-and-materials: "Lights are world-anchored").
- [x] 5.2 Add `light0On`/`light1On` booleans, `1`/`2` key toggles calling `glEnable`/`glDisable(GL_LIGHTx)`, and a very low `GL_LIGHT_MODEL_AMBIENT`. Verify: `1` and `2` each change only their own light, and with both off the scene is near-black without crashing (lighting-and-materials: all three toggle scenarios).
- [x] 5.3 Draw an emissive `glutSolidSphere` marker at each light position, emission set to the light colour when on and near-zero when off, resetting `GL_EMISSION` to zero afterward. Verify: markers glow when their light is on, dim when off, and the next object drawn is not glowing (lighting-and-materials: "Marker reflects light state", "No material bleed").

## 6. Living-area objects

- [x] 6.1 Implement `drawSofa()` — base, backrest, two armrests, 3 separated cushions, `MAT_FABRIC` — modelled at local origin facing −Z, wrapped in push/pop. Verify: from the front all five part groups are individually distinguishable (room-scene: "Sofa composition").
- [x] 6.2 Implement `drawCoffeeTable()` — tabletop + four legs in `MAT_WOOD`. Verify: from a low angle legs and top are separate and the table rests on the floor without intersecting the sofa (room-scene: "Coffee table structure").
- [x] 6.3 Implement `drawTV()` — wall bracket, bezel box, and inset screen quad; `MAT_METAL` mount, dark bezel, `MAT_GLASS` screen. Verify: the TV reads as attached to the wall with the bezel framing the screen (room-scene: "TV is wall-mounted and readable").
- [x] 6.4 Generate the procedural screen texture at startup (`glTexImage2D` from a code-built checker/gradient array), bind it only for the screen quad, disable texturing after; keep the `#define SCREEN_TEXTURE 0` flat-emissive fallback. Verify: the screen surface is clearly different from the bezel and toggling the define still builds and looks distinct.
- [x] 6.5 Implement `drawRug()` — thin box in `MAT_RUG`, offset 0.01 above the floor — placed under the sofa/coffee-table group. Verify: the rug is visible under the living furniture with no z-fighting flicker on the floor (room-scene: "Rug placement").

## 7. Dining-area objects

- [x] 7.1 Implement `drawDiningTable()` — rectangular top + exactly four legs in `MAT_WOOD`. Verify: four legs and a rectangular top are visible and nothing intersects the floor (room-scene: "Dining table structure").
- [x] 7.2 Implement `drawChair()` in a canonical local frame (floor-centre origin, backrest on the −Z side) with seat, four legs, and backrest. Verify: a single chair shows seat, legs, and backrest as separate parts (room-scene: "Chair composition").
- [x] 7.3 Place four chairs around the dining table in a loop with per-side `glTranslatef` + `glRotatef` (N/S/E/W), each inside its own push/pop. Verify: from above one chair sits on each side, all backrests point away from the table, and none intersects the table or another chair (room-scene: "Chair placement and orientation").

## 8. Ceiling fan and animation

- [x] 8.1 Implement `drawCeilingFan()` — down-rod, `gluCylinder` motor housing (`MAT_METAL`), and 4 blades in a child group. Verify: the fan hangs below the ceiling centre and blades clear the walls (room-scene: "Fan is mounted overhead").
- [x] 8.2 Apply `glRotatef(fanAngle, 0,1,0)` to the blade group only, inside its own push/pop. Verify: with `fanAngle` hardcoded to a few different values, blades rotate while the housing and rod stay fixed (fan-animation: "Blades spin in place").
- [x] 8.3 Implement `timer()` on `glutTimerFunc(16, …)` that re-registers itself, advances `fanAngle = fmod(fanAngle + step, 360)` when `fanOn`, and calls `glutPostRedisplay`. Verify: the fan completes ~1 revolution per second and the angle stays within 0–360 after minutes of running (fan-animation: "Consistent speed across machines", "Angle stays bounded").
- [x] 8.4 Bind `F`/`f` to toggle `fanOn`. Verify: pressing `f` freezes the blades at their current angle and pressing it again resumes from there with no jump (fan-animation: "Stop and resume").

## 9. Decor and fixtures

- [x] 9.1 Implement `drawFramedPicture()` — outer frame in `MAT_WOOD` around a contrasting inner surface — and place at least two on different walls, offset 0.01 off the wall. Verify: each picture sits flush with a visible frame and no z-fighting (room-scene: "Framed pictures").
- [x] 9.2 Implement `drawChandelier()` — rod, central body, and a few arms/bulb spheres in `MAT_METAL` — hanging over the dining table at the `GL_LIGHT0` position. Verify: the fixture is visible from below and coincides with the Light 0 marker (room-scene: "Light fixture present").

## 10. Scene assembly and overlay

- [x] 10.1 Write `drawScene()` calling every helper with its layout-constant placement, each in `glPushMatrix`/`glPopMatrix`. Verify: changing one object's anchor constant moves only that object (room-scene: "Object coordinate independence"), and living/dining groups occupy separate floor regions ("Areas are separated").
- [x] 10.2 Add a matrix-stack depth assertion around `drawScene()` (`glGetIntegerv(GL_MODELVIEW_STACK_DEPTH)` before/after) during development. Verify: depth is identical before and after and `glGetError()` reports no stack overflow/underflow (room-scene: "Matrix stack is balanced").
- [x] 10.3 Implement `drawOverlay()` — ortho projection, lighting and depth test disabled, `GLUT_BITMAP_9_BY_15` text listing key bindings and `Light0/Light1/Fan ON|OFF` — restoring all state afterward. Verify: toggling a light or the fan updates the overlay next frame, and the text stays readable with both lights off (application-shell: both overlay scenarios).
- [x] 10.4 Bind `Esc` to exit and make unbound keys no-ops. Verify: `Esc` closes the program cleanly and pressing e.g. `z` changes nothing (application-shell: "Escape exits", "Unknown key is a no-op").

## 11. Final verification and viva readiness

- [x] 11.1 Add descriptive comments over each matrix-transform block, the camera math, the light configuration, and each material constant. Verify: reading the file top-to-bottom explains every transformation without needing external notes.
- [x] 11.2 Build cleanly on macOS with the documented command and no warnings other than the suppressed deprecation ones. Verify: `clang++ main.cpp -o room -framework OpenGL -framework GLUT -Wno-deprecated-declarations` exits 0 (application-shell: "Builds from one command", "macOS build documented and works").
- [x] 11.3 Run a full manual acceptance pass over every spec scenario (room enclosure, all furniture present and composed, both light toggles, markers, fan toggle and speed, WASD/QE, mouse-look clamp, resize, Esc, overlay). Verify: every scenario in `specs/*/spec.md` is observed to hold, and note any deviation.
