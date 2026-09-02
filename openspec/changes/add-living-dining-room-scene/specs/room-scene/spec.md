## Purpose

Defines the bounded interior room and every hierarchically modeled furniture and decorative object the scene must present, along with the visual completeness expected of each object for a Computer Graphics viva.

## ADDED Requirements

### Requirement: Bounded room shell

The scene SHALL render an enclosed rectangular room consisting of a floor, a ceiling, and four walls that fully bound the visible space. Each surface SHALL have a single flat colour/material and SHALL have surface normals oriented toward the interior so that interior lighting evaluates correctly.

#### Scenario: Room is fully enclosed

- **WHEN** the camera is positioned anywhere inside the room and pointed at any of the six bounding surfaces
- **THEN** that surface is visible, lit, and shows no gap or hole to outside the room

#### Scenario: Interior-facing normals

- **WHEN** an enabled light illuminates a wall, the floor, or the ceiling from inside the room
- **THEN** the interior face of that surface is shaded as lit (not dark/back-faced)

### Requirement: Distinct living and dining areas

The room SHALL be visually partitioned into a living area and a dining area, each occupying a separate region of the floor plan, so that all living-area objects are grouped together and all dining-area objects are grouped together.

#### Scenario: Areas are separated

- **WHEN** the full room is viewed from above or from a far corner
- **THEN** the sofa, coffee table, TV, and rug are clustered in one region and the dining table with its chairs is clustered in a different, non-overlapping region

### Requirement: Wall-mounted flat-screen TV

The living area SHALL contain a flat-screen TV mounted on a wall, composed of at least an outer bezel, a mount/bracket connecting it to the wall, and a display surface. The display surface SHALL be visually distinct from the bezel via either a procedurally generated pattern or a contrasting emissive material. No external image file SHALL be required.

#### Scenario: TV is wall-mounted and readable

- **WHEN** the camera faces the TV wall
- **THEN** the TV appears attached to the wall (not floating and not on the floor), the bezel frames the display, and the display surface is clearly a different appearance from the bezel

### Requirement: Coffee/tea table

The living area SHALL contain a low table with a distinct tabletop and defined legs, positioned within reach of the sofa.

#### Scenario: Coffee table structure

- **WHEN** the coffee table is viewed from a low angle
- **THEN** separate legs and a tabletop are distinguishable, and the table stands on the floor without intersecting the sofa

### Requirement: Multi-cushion sofa

The living area SHALL contain a sofa composed of a base, a backrest, two armrests, and two or more visually separated seat cushions.

#### Scenario: Sofa composition

- **WHEN** the sofa is viewed from the front
- **THEN** the base, backrest, both armrests, and multiple cushions are individually distinguishable

### Requirement: Dining table

The dining area SHALL contain a dining table with a rectangular top supported by four legs, standing on the floor.

#### Scenario: Dining table structure

- **WHEN** the dining table is viewed
- **THEN** a rectangular top and exactly four legs are visible and the table does not intersect the floor or chairs

### Requirement: Four dining chairs on the four sides

The dining area SHALL contain four chairs, one centred on each of the North, South, East, and West sides of the dining table, each facing the table. Each chair SHALL have a seat base, legs, and a backrest, with the backrest on the outward-facing side.

#### Scenario: Chair placement and orientation

- **WHEN** the dining set is viewed from above
- **THEN** one chair is present on each side of the table, each chair's backrest points away from the table, and no chair intersects the table or another chair

#### Scenario: Chair composition

- **WHEN** a single dining chair is viewed
- **THEN** its seat, legs, and backrest are individually distinguishable

### Requirement: Ceiling fan geometry

The room SHALL contain a ceiling fan hanging from the ceiling near the centre of the room (or the living area), composed of a cylindrical motor housing and three or more blades arranged radially.

#### Scenario: Fan is mounted overhead

- **WHEN** the camera looks up toward the ceiling centre
- **THEN** the fan motor housing hangs below the ceiling and the blades extend radially from it without touching the walls

### Requirement: Decorative props

The scene SHALL include: a rug/carpet lying flat on the floor beneath the living-area furniture grouping; at least two framed pictures on the walls, each with a frame and a distinct picture surface; and a hanging chandelier or ceiling lamp fixture.

#### Scenario: Rug placement

- **WHEN** the living area is viewed
- **THEN** a flat rug sits on the floor under the sofa/coffee-table grouping and is visually distinct from the floor

#### Scenario: Framed pictures

- **WHEN** a wall with a picture is viewed
- **THEN** the picture is flush against the wall with a visible frame around a distinct inner surface

#### Scenario: Light fixture present

- **WHEN** the camera looks toward the ceiling
- **THEN** a chandelier/lamp fixture is visible hanging from the ceiling

### Requirement: Hierarchical modelling with balanced matrix stack

Every composite object SHALL be drawn by a dedicated helper routine that places the object using its own local coordinate frame, and SHALL wrap its transforms so that the OpenGL modelview matrix stack is left exactly as it was found. Drawing any object SHALL NOT change the position or orientation of any other object.

#### Scenario: Matrix stack is balanced

- **WHEN** the full scene has been drawn for a frame
- **THEN** the modelview matrix stack depth is identical to its depth before the scene was drawn, with no stack overflow or underflow reported

#### Scenario: Object coordinate independence

- **WHEN** the local transform of one object (e.g. the sofa) is changed
- **THEN** no other object in the scene changes position, orientation, or scale
