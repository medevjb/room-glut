## Purpose

Defines the GLUT application lifecycle, the consolidated input map, the on-screen help/status overlay, and the cross-platform build recipe that turns the single source file into a running program.

## ADDED Requirements

### Requirement: Single self-contained source file

The deliverable SHALL be one C++ source file (`main.cpp`) that compiles as a single translation unit and requires only the system OpenGL, GLU, and GLUT/freeglut libraries. It SHALL NOT require any external asset files, third-party libraries, or build system beyond a single compiler invocation.

#### Scenario: Builds from one command

- **WHEN** a grader compiles `main.cpp` with the documented compiler command on a supported platform
- **THEN** an executable is produced with no missing-file or missing-library errors

### Requirement: Documented cross-platform build recipe

The source file's header comment and the project tasks SHALL document build commands for macOS (primary: Clang, `-framework OpenGL -framework GLUT`), Linux (`g++ main.cpp -o app -lGL -lGLU -lglut`), and Windows (freeglut with MSVC or MinGW). Code SHALL include the correct GLUT header for the platform.

#### Scenario: macOS build documented and works

- **WHEN** a grader on macOS follows the documented macOS command
- **THEN** the program compiles and launches a window showing the room

#### Scenario: Linux/Windows notes present

- **WHEN** a grader reads the header comment
- **THEN** the Linux and Windows link commands are shown

### Requirement: GLUT application lifecycle

The program SHALL create a double-buffered, depth-tested window with a title, register display / reshape / keyboard / mouse-motion / timer callbacks, enable lighting and normalization of normals, and enter the GLUT main loop. The display callback SHALL clear colour and depth buffers, build the view, draw the scene, and swap buffers.

#### Scenario: Window opens ready to use

- **WHEN** the program starts
- **THEN** a titled window appears showing the lit room with no z-fighting artifacts on the walls/floor and the camera at a sensible starting position inside the room

### Requirement: Consolidated input map and quit

All controls SHALL be handled and SHALL be: `1`/`2` light toggles; `F`/`f` fan toggle; `W`/`A`/`S`/`D`/`Q`/`E` (any case) camera translation; mouse motion for look; `Esc` to exit the program. Unrecognized keys SHALL be ignored without side effects.

#### Scenario: Escape exits

- **WHEN** the user presses `Esc`
- **THEN** the program exits cleanly

#### Scenario: Unknown key is a no-op

- **WHEN** the user presses a key with no binding
- **THEN** nothing in the scene changes and no error occurs

### Requirement: On-screen help and status overlay

The program SHALL render a 2D text overlay (screen-space, unaffected by the camera or lighting) listing the key bindings and showing the current ON/OFF state of Light 0, Light 1, and the fan animation.

#### Scenario: Overlay shows current state

- **WHEN** the user toggles a light or the fan
- **THEN** the overlay's corresponding ON/OFF indicator updates on the next frame

#### Scenario: Overlay is always legible

- **WHEN** both lights are off and the scene is dark
- **THEN** the overlay text remains readable (drawn without lighting)
