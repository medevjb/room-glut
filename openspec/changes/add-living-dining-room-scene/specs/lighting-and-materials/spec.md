## Purpose

Defines the two independently toggleable OpenGL light sources, their on-screen markers and keyboard controls, and the per-material and surface-normal requirements that make Phong lighting evaluate realistically across wood, fabric, metal, and glass surfaces.

## ADDED Requirements

### Requirement: Two independent light sources

The scene SHALL be lit by exactly two configurable light sources: Light 0, a ceiling chandelier/grid light contributing ambient and diffuse illumination from above; and Light 1, a focused warm-toned spot or point light (e.g. floor lamp or TV/dining spotlight). Each light SHALL be positioned in world space each frame so that it stays fixed to its fixture regardless of camera movement.

#### Scenario: Both lights contribute when enabled

- **WHEN** both lights are enabled
- **THEN** the scene shows overall illumination from above plus a distinguishable warmer, more localized pool of light from Light 1

#### Scenario: Lights are world-anchored

- **WHEN** the camera moves or rotates while a light is enabled
- **THEN** the lit and shadowed sides of objects change consistently with a light that stays at its fixture, not one that follows the camera

### Requirement: Independent keyboard toggles

Pressing `1` SHALL toggle Light 0 between ON and OFF. Pressing `2` SHALL toggle Light 1 between ON and OFF. The two toggles SHALL be independent, and toggling one SHALL NOT affect the other.

#### Scenario: Toggle Light 0

- **WHEN** the user presses `1`
- **THEN** Light 0's contribution is added if it was off, or removed if it was on, with Light 1 unchanged

#### Scenario: Toggle Light 1

- **WHEN** the user presses `2`
- **THEN** Light 1's contribution is added if it was off, or removed if it was on, with Light 0 unchanged

#### Scenario: All lights off

- **WHEN** both lights are toggled off
- **THEN** the scene renders very dark (only any global ambient / emissive surfaces remain visible) and no crash occurs

### Requirement: Emissive light markers

Each light source SHALL have a small geometric marker (e.g. a sphere) drawn at its position using an emissive material so it appears to glow. When a light is OFF, its marker SHALL visibly dim or stop glowing to indicate the state.

#### Scenario: Marker reflects light state

- **WHEN** a light is enabled
- **THEN** a glowing marker is visible at that light's fixture position
- **AND WHEN** that light is disabled
- **THEN** the same marker appears dim / non-glowing

### Requirement: Per-material surface properties

Objects SHALL be assigned material properties via ambient, diffuse, specular, and shininess terms such that at least four distinct material appearances are present: matte-to-satin wood (tables, chairs, frames), low-specular fabric (sofa, cushions, rug), high-specular metal (fan blades, mounts, fixture), and glossy screen glass (TV display). Selecting a material for one object SHALL NOT leak into subsequently drawn objects that set their own material.

#### Scenario: Materials are visually distinct

- **WHEN** the scene is lit and viewed
- **THEN** wood surfaces show a soft highlight, fabric surfaces show little to no highlight, metal surfaces show a tight bright highlight, and the TV glass shows a broad glossy highlight

#### Scenario: No material bleed

- **WHEN** an object that sets its own material is drawn immediately after an object with a very different material
- **THEN** the later object renders with only its own material properties

### Requirement: Correct surface normals

All planar and composite geometry SHALL specify surface normals consistent with each face's outward orientation so that diffuse and specular lighting vary correctly with angle. Flat faces SHALL use a single face normal; primitives generated with normals (spheres, cylinders) SHALL retain them.

#### Scenario: Lighting varies across orientations

- **WHEN** a single light illuminates a cube-shaped object from one side
- **THEN** faces directly facing the light are brightest, side faces are dimmer, and the face away from the light is darkest

#### Scenario: No inside-out shading

- **WHEN** any object surface that should face the viewer is viewed under enabled lighting
- **THEN** it is shaded as a front face (not black due to an inverted normal)
