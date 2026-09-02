## Context

See `proposal.md` — Why. Fresh project, no existing code. Deliverable is one `main.cpp` built with a single compiler command, primary platform macOS (Clang + system GLUT / freeglut), also compiling on Linux and Windows. Fixed-function OpenGL only (GLUT/GLU), because the assignment is explicitly "Classic OpenGL" and must be viva-defensible with `glPushMatrix`/`glMaterialfv`/`gluLookAt` visible in the source. Requirements live in `specs/*/spec.md`.

## Goals / Non-Goals

**Goals:**

- One flat, readable source file with clearly commented sections mapping 1:1 to the five capabilities.
- A world-coordinate layout table so every object's placement is deterministic and documented for the viva.
- Small, composable draw helpers that each leave the matrix stack balanced.
- Deterministic, frame-rate-independent fan animation.

**Non-Goals:**

- Shaders / VBOs / modern GL pipeline.
- Shadows, reflections, global illumination, post-processing.
- Loading external textures, models, or fonts (procedural screen only; GLUT bitmap font for overlay).
- Collision detection — the camera may pass through geometry and walls.
- Resizable furniture or a scene-graph library; hierarchy is expressed directly through nested `glPushMatrix`/`glTranslate`/`glRotate` calls.

## Decisions

### D1: Fixed-function pipeline, GLUT primitives + a `drawBox` helper

Use `glEnable(GL_LIGHTING)`, `glMaterialfv`, `glutSolidCube/Sphere`, `gluCylinder`/`glutSolidCone` for the fan/fixtures, and one custom `drawBox(w,h,d)` that emits 6 quads with explicit per-face `glNormal3f` for all rectangular parts (walls, tabletops, cushions, frames). `glutSolidCube` only gives a unit cube, so `drawBox` (scaled quads with correct normals) is the workhorse; `glEnable(GL_NORMALIZE)` covers non-uniform scaling elsewhere.
_Alternative:_ `glScalef` + `glutSolidCube` everywhere — rejected because non-uniform scale distorts lighting unless normalized and gives no control over face-level normals, which the spec grades.

### D2: World layout and coordinate frame

Right-handed, Y up, units = metres. Room interior roughly 12 (X) × 3.2 (Y) × 9 (Z), floor at y=0, centred on origin. Living area occupies −X half, dining area +X half. A single commented constants block at the top of `main.cpp` holds room dimensions and each object's anchor position + facing angle. Every draw helper models its object in a canonical local frame (origin at the object's floor-centre, facing −Z) and the scene function positions it.

| Object | Anchor (x,y,z) | Facing | Helper |
|---|---|---|---|
| Room shell | 0,0,0 | — | `drawRoom` |
| Sofa | −4.2, 0, −3.0 | +Z (faces room centre) | `drawSofa` |
| Coffee table | −4.2, 0, −1.2 | — | `drawCoffeeTable` |
| TV | −5.9 (on −X wall), 1.6, −3.0 | +X | `drawTV` |
| Rug | −4.2, 0.01, −2.0 | — | `drawRug` |
| Dining table | 3.5, 0, 0 | — | `drawDiningTable` |
| Dining chairs ×4 | table ± (1.1 in X or Z) | toward table | `drawChair` in a loop |
| Ceiling fan | 0, 3.2, 0 | — | `drawCeilingFan` |
| Chandelier | 3.5, 3.2, 0 | — | `drawChandelier` |
| Wall pictures ×2+ | on +Z and −X walls | into room | `drawFramedPicture` |

### D3: Camera as (position, yaw, pitch)

Store `Vec3 camPos`, `float yaw, pitch`. Each frame compute `forward = (cos(pitch)sin(yaw), sin(pitch), -cos(pitch)cos(yaw))`, `right = normalize(cross(forward, worldUp))`, then `gluLookAt(camPos, camPos+forward, worldUp)`. WASD adds `±forward`/`±right * speed`; Q/E adds `±worldUp * speed`. Movement handled in the keyboard callback with a fixed step (simple, deterministic, good enough for a demo); a held-key set + timer integration is noted as an optional refinement in tasks but not required by the spec.
_Mouse:_ `glutPassiveMotionFunc` reads delta from window centre, applies `yaw += dx*sens`, `pitch -= dy*sens`, clamps pitch to ±89°, then `glutWarpPointer` back to centre. A re-entrancy guard flag prevents the warp from being processed as user input.
_Alternative:_ arrow-key look — kept as a fallback binding but mouse-look is primary since the spec lists it first.

### D4: Two lights, world-anchored, re-specified every frame

`GL_LIGHT0`: positional at the chandelier (`w=1`), whitish, moderate ambient + diffuse, low specular — the room fill. `GL_LIGHT1`: `GL_SPOT` at a floor-lamp/TV position, warm RGB (~1.0, 0.8, 0.55), `GL_SPOT_CUTOFF` ~35°, `GL_SPOT_EXPONENT` ~12, with attenuation — the accent. Positions are set **after** `gluLookAt` each frame so they live in world space, not eye space. Booleans `light0On`, `light1On` gate `glEnable/glDisable(GL_LIGHTx)`. `GL_LIGHT_MODEL_AMBIENT` kept very low so "all lights off" is nearly black but not pure black (spec scenario).
Markers: a `glutSolidSphere` at each light position drawn with `GL_EMISSION` set to the light's colour when on, near-zero when off; emission reset to 0 afterward so it never leaks (spec: no material bleed).

### D5: Materials via a `setMaterial(spec)` helper

A small `struct Material { float amb[4], dif[4], spec[4], shin; }` and `setMaterial(const Material&)` calling four `glMaterialfv/​glMaterialf` on `GL_FRONT`. Named constants: `MAT_WOOD` (satin, shininess ~24), `MAT_FABRIC` (spec ~0.05, shininess ~4), `MAT_METAL` (spec ~0.9, shininess ~96), `MAT_GLASS` (spec ~0.8, shininess ~64, slight emission for the screen), plus `MAT_WALL`, `MAT_FLOOR`, `MAT_RUG`. Every helper calls `setMaterial` before drawing, so state is always explicit — this directly answers the "no material bleed" scenario.

### D6: Procedural TV screen

Generate a small RGB checker/gradient array once at startup into a `GLuint` texture (`glTexImage2D`), bind it only while drawing the screen quad with `GL_TEXTURE_2D` enabled, disable texturing afterward. If a grader's driver disliked texturing, a `#define SCREEN_TEXTURE 0` fallback swaps to a flat contrasting emissive material. Either way self-contained.

### D7: Fan animation timing

`glutTimerFunc(16, tick, 0)` (~60 Hz). `tick` re-registers itself and, if `fanOn`, does `fanAngle = fmod(fanAngle + FAN_DEG_PER_TICK, 360.0f)` then `glutPostRedisplay()`. `FAN_DEG_PER_TICK` chosen for ~1 rev/sec. Because the increment is per scheduled tick (fixed interval), speed is wall-clock consistent regardless of render cost. `drawCeilingFan` applies `glRotatef(fanAngle, 0,1,0)` to the blade group only, inside its own push/pop, so the housing stays put and the fan stays mounted.

### D8: Overlay

`glutBitmapString`/`glutBitmapCharacter` with `GLUT_BITMAP_9_BY_15`, drawn in an ortho projection block with lighting and depth-test disabled, then state restored. Lists key bindings and `Light0: ON/OFF`, `Light1: ON/OFF`, `Fan: ON/OFF` from the state booleans.

## Risks / Trade-offs

- **Apple deprecated GLUT/OpenGL headers emit build warnings** → wrap includes with the `__APPLE__` branch (`<GLUT/glut.h>`, `<OpenGL/gl.h>`) and document `-Wno-deprecated-declarations`; functionality is unaffected on current macOS.
- **`glutWarpPointer` can cause mouse-look jitter / recursion** → centre-warp only when the delta is non-zero, guard with an `ignoreNextMotion` flag, ignore the first motion event after focus.
- **Non-uniform `glScalef` distorting normals** → prefer `drawBox` with real normals; keep `GL_NORMALIZE` enabled globally as a safety net.
- **Spot light looks flat because room faces are single quads (few vertices to interpolate)** → subdivide floor/walls into an N×N grid of quads so per-vertex lighting from `GL_LIGHT1` shows a visible pool; N configurable, default ~10.
- **Z-fighting between rug and floor, pictures and walls** → offset decor by a small epsilon (0.01) along the surface normal, documented in the layout constants.
- **`glutBitmapString` missing in very old GLUT** → fall back to a per-character loop (both are in freeglut and macOS GLUT).

## Findings during implementation

Two things in the decisions above turned out to be wrong or incomplete and were corrected in code:

- **D2's anchor table put the sofa at `(-4.2, 0, -3.0)` facing +Z while the TV sat on the −X wall** — i.e. the sofa faced away from the TV. The implemented layout instead runs the living group along the X axis: TV on the −X wall at x≈−6, coffee table at x=−3.5, sofa at x=−1.9 rotated 90° to face −X, with the rug spanning the group. The dining set stays in the +X half.
- **A spot light with any `GL_SPOT_CUTOFF < 180` yields a NaN vertex colour on this GL implementation for vertices that sit *behind* the lamp with a normal parallel to its axis.** The NaN rasterises as pure black and wipes out even `GL_EMISSION`, which blacked out the entire ceiling and a band of the wall tops. `GL_SPOT_EXPONENT` is irrelevant; only the cutoff triggers it. The fix is layout-level and must be preserved: `GL_LIGHT1` is mounted at `ROOM_H + 0.06`, strictly above every polygon in the room, with its visible fixture drawn just below the ceiling, and the ceiling is drawn with `GL_LIGHT1` disabled (it is outside a downward cone by construction, and this also avoids a hotspot from the lamp being just above it). Lowering `SPOT_Y` back under the ceiling reintroduces the artifact.

## Migration Plan

Not applicable — additive, single new file, no existing behaviour. "Rollback" = delete `main.cpp`.

## Open Questions

None that affect specs, approach, or task breakdown.
