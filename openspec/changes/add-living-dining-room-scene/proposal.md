## Why

The Computer Graphics final assignment requires a self-contained C++/OpenGL (GLUT/GLU) program that demonstrates hierarchical modeling, lighting, materials, animation, and interactive camera navigation, defensible in a viva. The project currently has no source; this change plans a single `main.cpp` deliverable that satisfies every rubric item.

## What Changes

- Add a combined 3D **Living + Dining Room** interior: floor, ceiling, and four walls with correct outward/inward surface normals.
- Add hierarchically modeled furniture via modular helper functions:
  - Living area: wall-mounted flat-screen TV (bezel, mount, contrasting/procedural display), coffee/tea table, multi-cushion sofa (base, backrest, armrests).
  - Dining area: rectangular dining table with 4 legs, and 4 dining chairs (seat, legs, backrest) placed North/South/East/West around it.
  - Decor: living-area rug, framed wall pictures, chandelier/ceiling lamp fixture.
- Add a **ceiling fan** (motor housing cylinder + blades) with continuous Y-axis blade rotation driven by `glutTimerFunc`, toggled with `F`/`f`.
- Add **two independent toggleable light sources**: `GL_LIGHT0` (ceiling chandelier, ambient/diffuse) and `GL_LIGHT1` (warm focused spot/point light), each with an emissive geometric marker; toggled with `1` and `2`.
- Add per-object **material definitions** (`GL_AMBIENT`/`GL_DIFFUSE`/`GL_SPECULAR`/`GL_SHININESS`) distinguishing wood, fabric, metal, and screen glass, with explicit `glNormal3f` normals on all planar/composite geometry.
- Add a **free-fly first-person camera**: WASD translation, `Q`/`E` vertical move, mouse-motion yaw/pitch, `gluLookAt` recomputed every frame.
- Add a **build recipe** for macOS (Clang, `-framework OpenGL -framework GLUT`) as primary, with Linux (`-lGL -lGLU -lglut`) and Windows (freeglut) notes.
- The TV display surface is **procedural / self-contained** (generated checker/gradient or contrasting emissive material) — no external asset files. `main.cpp` is the only deliverable.

## Capabilities

### New Capabilities

- `room-scene`: The bounded room shell and all hierarchically modeled furniture and decorative props, their placement, and the matrix-stack discipline used to draw them.
- `lighting-and-materials`: The two toggleable OpenGL light sources, their emissive markers, keyboard toggles, and the per-material property sets and vertex normals that make Phong shading evaluate correctly.
- `fan-animation`: The timer-driven continuous rotation of the ceiling fan blades and its toggle.
- `camera-navigation`: The free-fly camera state, WASD/QE translation, mouse-look orientation, and per-frame `gluLookAt` update.
- `application-shell`: The GLUT application lifecycle (window, callbacks, projection, main loop), the consolidated keyboard/mouse input map, an on-screen help/status overlay, and the cross-platform build recipe.

### Modified Capabilities

- None (new project).

## Impact

- New file: `main.cpp` (single translation unit) at the project root.
- New/updated build docs: compile command in `main.cpp` header comment and in `tasks.md`.
- Dependencies: system OpenGL, GLU, and GLUT/freeglut. No package manager or third-party libraries.
- No runtime assets, no network, no persistence.
