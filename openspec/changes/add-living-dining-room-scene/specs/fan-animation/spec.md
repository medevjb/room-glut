## Purpose

Defines the continuous, timer-driven rotation of the ceiling fan blades and the control that starts and stops it, independent of the machine's frame rate.

## ADDED Requirements

### Requirement: Continuous blade rotation

While animation is enabled, the ceiling fan blades SHALL rotate continuously about the fan's local vertical (Y) axis, passing through the motor housing centre. The motor housing SHALL NOT rotate. Rotation SHALL NOT displace the fan from its mounting point.

#### Scenario: Blades spin in place

- **WHEN** animation is enabled and several frames elapse
- **THEN** the blades are at a visibly different angular position each frame while the motor housing and mount stay fixed

### Requirement: Frame-rate independent timing

Blade angular velocity SHALL be driven by a GLUT timer callback at a fixed scheduled interval so that the fan completes a full revolution in approximately the same wall-clock time regardless of rendering speed. The rotation angle SHALL wrap (modulo 360°) to avoid unbounded growth.

#### Scenario: Consistent speed across machines

- **WHEN** the program runs on a fast machine and on a slow machine, both with animation enabled
- **THEN** the fan's revolutions-per-second are approximately equal on both

#### Scenario: Angle stays bounded

- **WHEN** the animation has run for a long time
- **THEN** the stored rotation angle remains within a bounded range (0–360°) and no visual stutter from precision loss occurs

### Requirement: Animation toggle

Pressing `F` or `f` SHALL toggle the fan animation between running and stopped. When stopped, the blades SHALL hold their current angle. When restarted, rotation SHALL resume smoothly from the held angle without a jump.

#### Scenario: Stop and resume

- **WHEN** the user presses `f` while the fan is spinning
- **THEN** the blades freeze at their current position
- **AND WHEN** the user presses `f` again
- **THEN** the blades resume spinning from that same position at the previous speed
