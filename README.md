# 3D Living + Dining Room — OpenGL (GLUT / GLU)

A single-file C++ OpenGL scene: a bounded living + dining room interior with
hierarchical furniture, two independently toggleable light sources, an animated
ceiling fan, and a free-fly first-person camera.

Everything lives in **`main.cpp`**. There are no external assets, no third-party
libraries, and no build system — one compiler command produces the program.

---

## 1. Requirements

| Platform | You need |
|---|---|
| macOS | Xcode Command Line Tools (`xcode-select --install`). OpenGL, GLU and GLUT ship with the OS. |
| Linux | A C++ compiler plus the OpenGL/GLU/freeglut development packages. |
| Windows | MSVC or MinGW, plus [freeglut](https://freeglut.sourceforge.net/). |

Installing the dependencies on Linux:

```bash
# Debian / Ubuntu
sudo apt install build-essential freeglut3-dev libglu1-mesa-dev

# Fedora / RHEL
sudo dnf install gcc-c++ freeglut-devel mesa-libGLU-devel

# Arch
sudo pacman -S base-devel freeglut glu
```

---

## 2. Compile and run

### macOS

```bash
clang++ main.cpp -o room -framework OpenGL -framework GLUT
./room
```

macOS marks the fixed-function pipeline deprecated. `main.cpp` already defines
`GL_SILENCE_DEPRECATION`, so the command above compiles **without warnings** —
you do not need a suppression flag. If you build in an environment that reports
them anyway, add `-Wno-deprecated-declarations`.

### Linux

```bash
g++ main.cpp -o room -lGL -lGLU -lglut
./room
```

### Windows — MSVC

```bat
cl main.cpp /I<path-to-freeglut>\include ^
   /link <path-to-freeglut>\lib\freeglut.lib opengl32.lib glu32.lib
room.exe
```

Copy `freeglut.dll` next to the executable, or put it on your `PATH`.

### Windows — MinGW

```bash
g++ main.cpp -o room.exe -lfreeglut -lopengl32 -lglu32
./room.exe
```

### Optional flags

The program is plain C++ with no standard-library version requirements — it
compiles under `-std=c++98`, `c++11` and `c++17` alike, and is warning-free
under `-Wall -Wextra`:

```bash
clang++ -Wall -Wextra main.cpp -o room -framework OpenGL -framework GLUT
```

---

## 3. Controls

| Key / input | Action |
|---|---|
| `W` / `S` | Move forward / backward along the camera look vector |
| `A` / `D` | Strafe left / right |
| `Q` / `E` | Move up / down along the world Y axis |
| Mouse | Look around (yaw + pitch) |
| Arrow keys | Look around — alternative to the mouse |
| `1` | Toggle **Light 0** — the dining chandelier |
| `2` | Toggle **Light 1** — the warm accent spot over the living area |
| `F` | Toggle the ceiling fan animation |
| `Esc` | Quit |

All letter keys work in either case. Any unbound key is ignored.

> **The window captures your mouse.** Moving the mouse turns the view, and the
> pointer is warped back to the window centre each time. Press `Esc` to quit.

---

## 4. What you should see on start-up

A lit room with the camera inside, near the dining area looking back across the
scene:

- **Living half (−X):** wall-mounted TV with a generated screen image, sofa with
  separate cushions, coffee table, and a rug beneath the group.
- **Dining half (+X):** rectangular table with four legs and four chairs, one on
  each of the north, south, east and west sides.
- **Overhead:** a rotating ceiling fan, a chandelier with glowing bulbs, and a
  recessed downlight above the living area.
- **Top-left:** a text overlay listing the controls and the live ON/OFF state of
  both lights and the fan.

If the room looks almost black, you have toggled both lights off — press `1`.

---

## 5. Code map

`main.cpp` is organised into 13 numbered sections, each marked with a banner
comment:

| Section | Contents |
|---|---|
| 1 | Vector maths used by the camera |
| 2 | `Material` struct, `setMaterial()`, and the material palette |
| 3 | Geometry primitives — `drawBox()`, `drawPlaneGrid()` |
| 4 | World layout constants (every object's anchor position) |
| 5 | Room shell — floor, ceiling, four walls |
| 6 | Free-fly camera and `gluLookAt` |
| 7 | Lighting — `GL_LIGHT0`, `GL_LIGHT1`, emissive markers |
| 8 | Living-area furniture — sofa, coffee table, TV, rug |
| 9 | Dining-area furniture — table, chair |
| 10 | Ceiling fan and its rotating blade group |
| 11 | Decorative fixtures — pictures, chandelier, spot housing |
| 12 | Scene assembly |
| 13 | 2D help / status overlay |

To move an object, edit its anchor constant in **Section 4** — each object is
drawn inside its own `glPushMatrix()` / `glPopMatrix()` pair, so moving one
never disturbs another.

---

## 6. Troubleshooting

**`fatal error: GL/glut.h: No such file or directory` (Linux)**
The freeglut headers are missing — install the development packages in §1.

**`Undefined symbols: _glBegin, _glutInit, …` (macOS)**
You left off the frameworks. Both `-framework OpenGL` and `-framework GLUT` are
required.

**`unresolved external symbol __imp_glutInit` (Windows)**
freeglut is not being linked, or you are mixing a 32-bit library with a 64-bit
build. Make sure the freeglut `lib` you point at matches your compiler target.

**The window opens but stays black**
Both lights are off — press `1` and `2`. The overlay text stays readable even
when the room is dark, so use it to confirm the current state.

**The view spins away on its own / the pointer is stuck**
The mouse is captured for look control by design. Press `Esc` to quit.

---

## 7. A note on the accent spot light

`GL_LIGHT1` is deliberately mounted *above* the ceiling plane
(`SPOT_Y = ROOM_H + 0.06`), and the ceiling is drawn with that light disabled.

This is not arbitrary. On some drivers — Apple's GL among them — a spot light
with any `GL_SPOT_CUTOFF` below 180° produces a NaN vertex colour for geometry
that sits *behind* the lamp with a surface normal parallel to the spot axis.
The NaN rasterises as pure black and overrides even `GL_EMISSION`, which blacks
out the whole ceiling. Keeping the emitter above every polygon in the room
avoids the case entirely.

**Lowering `SPOT_Y` back under the ceiling will bring the artifact back.** The
constant carries the same warning in the source.
