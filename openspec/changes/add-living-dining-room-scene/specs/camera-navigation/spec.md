## Purpose

Defines the interactive free-fly first-person camera: how the viewer translates and rotates through the room and how the view transform is rebuilt each frame with gluLookAt.

## ADDED Requirements

### Requirement: Free-fly camera state

The camera SHALL maintain an eye position in world space and a viewing orientation expressed as yaw and pitch angles. From these it SHALL derive, each frame, a normalized forward vector, a right vector, and a world-up vector, and SHALL set the view transform via `gluLookAt(eye, eye + forward, up)`.

#### Scenario: View follows camera state

- **WHEN** the camera's eye position or yaw/pitch changes
- **THEN** the next rendered frame shows the scene from the new position/orientation

### Requirement: WASD and vertical translation

The following keys SHALL move the camera by a fixed step per press (or per frame while held), and SHALL work regardless of the current view direction:

- `W`/`w`: move forward along the camera forward vector
- `S`/`s`: move backward along the camera forward vector
- `A`/`a`: strafe left along the camera right vector
- `D`/`d`: strafe right along the camera right vector
- `Q`/`q`: move up along world +Y
- `E`/`e`: move down along world -Y

#### Scenario: Forward moves along look direction

- **WHEN** the camera is pitched and yawed arbitrarily and the user presses `W`
- **THEN** the eye position moves in the direction the camera is currently looking

#### Scenario: Strafe is perpendicular to look direction

- **WHEN** the user presses `A` or `D`
- **THEN** the eye position moves left or right perpendicular to the forward vector, without changing orientation

#### Scenario: Vertical move is world-aligned

- **WHEN** the user presses `Q` or `E`
- **THEN** the eye position moves straight up or down along the world Y axis regardless of pitch

### Requirement: Mouse-look orientation

Moving the mouse SHALL adjust yaw (horizontal) and pitch (vertical) proportionally to the mouse displacement. Pitch SHALL be clamped to just under ±90° so the view never flips over. Yaw SHALL be unbounded (wrapping is acceptable).

#### Scenario: Mouse turns the view

- **WHEN** the user moves the mouse left/right
- **THEN** the view yaws left/right; moving the mouse up/down pitches the view up/down

#### Scenario: Pitch cannot flip

- **WHEN** the user pushes the mouse continuously up or down
- **THEN** the pitch stops at just under straight up / straight down and the horizon never inverts

### Requirement: Perspective projection maintained on resize

The camera SHALL render with a perspective projection whose aspect ratio matches the window. When the window is resized, the projection SHALL be updated so the scene is not stretched.

#### Scenario: Resize keeps proportions

- **WHEN** the window is resized to a new width/height
- **THEN** objects keep their proportions (a square face still looks square) and the full viewport is used
