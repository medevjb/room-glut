// =============================================================================
//  3D Living + Dining Room Interior  —  Classic OpenGL (GLUT / GLU)
//  Computer Graphics final assignment
// =============================================================================
//
//  A single, self-contained translation unit. No external assets, no third
//  party libraries, no build system: one compiler invocation.
//
//  BUILD
//  -----
//    macOS   (primary):
//      clang++ main.cpp -o room -framework OpenGL -framework GLUT \
//              -Wno-deprecated-declarations
//
//    Linux:
//      g++ main.cpp -o room -lGL -lGLU -lglut
//
//    Windows (MSVC + freeglut):
//      cl main.cpp /I<freeglut>\include /link <freeglut>\lib\freeglut.lib \
//         opengl32.lib glu32.lib
//    Windows (MinGW + freeglut):
//      g++ main.cpp -o room.exe -lfreeglut -lopengl32 -lglu32
//
//  CONTROLS
//  --------
//    W / S ....... move forward / backward along the camera look vector
//    A / D ....... strafe left / right (perpendicular to the look vector)
//    Q / E ....... move up / down along the world Y axis
//    Mouse ....... look around (yaw + pitch); arrow keys do the same
//    1 ........... toggle Light 0 (ceiling chandelier)  ON / OFF
//    2 ........... toggle Light 1 (warm spot light)     ON / OFF
//    F ........... toggle the ceiling fan animation
//    Esc ......... quit
//
//  NOTE ON PORTABILITY
//  -------------------
//  Apple ships the original GLUT 3.x, which lacks the freeglut extras
//  glutSolidCylinder() and glutBitmapString(). This file therefore uses
//  gluCylinder() for cylinders and a per-character glutBitmapCharacter()
//  loop for text, both of which exist everywhere.
// =============================================================================

#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION      // macOS 10.14+ marks the fixed pipeline deprecated
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// -----------------------------------------------------------------------------
//  Window
// -----------------------------------------------------------------------------
static int   winWidth  = 1200;
static int   winHeight = 800;

// -----------------------------------------------------------------------------
//  Runtime state toggled from the keyboard (see keyboard() at the bottom).
// -----------------------------------------------------------------------------
static bool  light0On  = true;    // '1' - ceiling chandelier   (GL_LIGHT0)
static bool  light1On  = true;    // '2' - warm accent spot     (GL_LIGHT1)
static bool  fanOn     = true;    // 'F' - ceiling fan animation
static float fanAngle  = 0.0f;    // current blade rotation, degrees

// =============================================================================
//  SECTION 1 — SMALL VECTOR MATH  (used by the camera)
// =============================================================================
struct Vec3
{
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}
};

static inline Vec3 add(const Vec3& a, const Vec3& b)
{
    return Vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline Vec3 scale(const Vec3& a, float s)
{
    return Vec3(a.x * s, a.y * s, a.z * s);
}

// Right-handed cross product: cross(forward, up) yields the camera "right".
static inline Vec3 cross(const Vec3& a, const Vec3& b)
{
    return Vec3(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}

static inline Vec3 normalize(const Vec3& a)
{
    float len = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
    if (len < 1e-6f) return Vec3(0, 0, 0);
    return Vec3(a.x / len, a.y / len, a.z / len);
}

// =============================================================================
//  SECTION 2 — MATERIALS
//  Every draw helper sets its material explicitly before emitting geometry.
//  setMaterial() always writes ALL five properties (emission included), so no
//  property can leak from one object into the next.
// =============================================================================
struct Material
{
    GLfloat amb[4];     // GL_AMBIENT   - colour under indirect light
    GLfloat dif[4];     // GL_DIFFUSE   - main colour under direct light
    GLfloat spec[4];    // GL_SPECULAR  - highlight colour
    GLfloat emi[4];     // GL_EMISSION  - self-glow (0 for everything but lamps)
    GLfloat shin;       // GL_SHININESS - highlight tightness, 0..128
};

static void setMaterial(const Material& m)
{
    // GL_FRONT_AND_BACK keeps interior-facing geometry correct even if a face
    // is viewed from behind (e.g. standing outside the room shell).
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   m.amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   m.dif);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  m.spec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  m.emi);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, m.shin);
}

// --- The material palette -------------------------------------------------
// Four clearly distinguishable families, as required by the spec:
//   wood   = satin, moderate tight-ish highlight
//   fabric = matte, almost no highlight
//   metal  = very bright, very tight highlight
//   glass  = broad glossy highlight (TV screen)
static const Material MAT_WOOD = {
    {0.14f, 0.09f, 0.04f, 1.0f}, {0.52f, 0.32f, 0.15f, 1.0f},
    {0.28f, 0.22f, 0.16f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, 24.0f };

static const Material MAT_WOOD_DARK = {
    {0.07f, 0.045f, 0.02f, 1.0f}, {0.26f, 0.16f, 0.08f, 1.0f},
    {0.22f, 0.18f, 0.13f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, 20.0f };

static const Material MAT_FABRIC = {
    {0.10f, 0.10f, 0.14f, 1.0f}, {0.34f, 0.34f, 0.46f, 1.0f},
    {0.04f, 0.04f, 0.05f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f},  4.0f };

static const Material MAT_FABRIC_CUSHION = {
    {0.12f, 0.11f, 0.16f, 1.0f}, {0.44f, 0.42f, 0.56f, 1.0f},
    {0.05f, 0.05f, 0.06f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f},  5.0f };

static const Material MAT_METAL = {
    {0.13f, 0.14f, 0.15f, 1.0f}, {0.46f, 0.49f, 0.53f, 1.0f},
    {0.92f, 0.94f, 0.97f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, 96.0f };

static const Material MAT_GLASS = {
    {0.02f, 0.03f, 0.04f, 1.0f}, {0.06f, 0.08f, 0.11f, 1.0f},
    {0.85f, 0.88f, 0.92f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, 64.0f };

static const Material MAT_WALL = {
    {0.20f, 0.19f, 0.17f, 1.0f}, {0.74f, 0.71f, 0.64f, 1.0f},
    {0.04f, 0.04f, 0.04f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f},  8.0f };

static const Material MAT_FLOOR = {
    {0.10f, 0.07f, 0.04f, 1.0f}, {0.40f, 0.28f, 0.17f, 1.0f},
    {0.22f, 0.18f, 0.14f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, 20.0f };

static const Material MAT_CEILING = {
    {0.20f, 0.20f, 0.20f, 1.0f}, {0.78f, 0.78f, 0.78f, 1.0f},
    {0.03f, 0.03f, 0.03f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f},  6.0f };

static const Material MAT_RUG = {
    {0.14f, 0.05f, 0.05f, 1.0f}, {0.52f, 0.17f, 0.15f, 1.0f},
    {0.03f, 0.03f, 0.03f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f},  3.0f };

static const Material MAT_DARK_PLASTIC = {
    {0.03f, 0.03f, 0.03f, 1.0f}, {0.10f, 0.10f, 0.11f, 1.0f},
    {0.35f, 0.35f, 0.38f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, 40.0f };

// =============================================================================
//  SECTION 3 — GEOMETRY PRIMITIVES
// =============================================================================

// A rectangular box centred on the local origin, built from six quads, each
// carrying its own explicit outward face normal so Phong lighting evaluates
// per-face rather than being inherited from whatever was set last.
static void drawBox(float w, float h, float d)
{
    const float x = w * 0.5f, y = h * 0.5f, z = d * 0.5f;

    glBegin(GL_QUADS);
        // +Z (front)
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f(-x, -y,  z); glVertex3f( x, -y,  z);
        glVertex3f( x,  y,  z); glVertex3f(-x,  y,  z);
        // -Z (back)
        glNormal3f(0.0f, 0.0f, -1.0f);
        glVertex3f( x, -y, -z); glVertex3f(-x, -y, -z);
        glVertex3f(-x,  y, -z); glVertex3f( x,  y, -z);
        // +X (right)
        glNormal3f(1.0f, 0.0f, 0.0f);
        glVertex3f( x, -y,  z); glVertex3f( x, -y, -z);
        glVertex3f( x,  y, -z); glVertex3f( x,  y,  z);
        // -X (left)
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glVertex3f(-x, -y, -z); glVertex3f(-x, -y,  z);
        glVertex3f(-x,  y,  z); glVertex3f(-x,  y, -z);
        // +Y (top)
        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(-x,  y,  z); glVertex3f( x,  y,  z);
        glVertex3f( x,  y, -z); glVertex3f(-x,  y, -z);
        // -Y (bottom)
        glNormal3f(0.0f, -1.0f, 0.0f);
        glVertex3f(-x, -y, -z); glVertex3f( x, -y, -z);
        glVertex3f( x, -y,  z); glVertex3f(-x, -y,  z);
    glEnd();
}

// A flat plane lying in the local XZ plane (y = 0), centred on the origin,
// subdivided into n x n quads. The subdivision matters: OpenGL's fixed pipeline
// evaluates lighting PER VERTEX, so a wall drawn as a single quad could only be
// lit at its four corners and the spot light would leave no visible pool on it.
// The caller rotates this plane into place for walls and the ceiling; `nx,ny,nz`
// lets a caller flip the normal without rotating.
static void drawPlaneGrid(float w, float d, int n, float nx, float ny, float nz)
{
    if (n < 1) n = 1;
    const float x0 = -w * 0.5f, z0 = -d * 0.5f;
    const float dx = w / n,     dz = d / n;

    glBegin(GL_QUADS);
        glNormal3f(nx, ny, nz);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                const float xa = x0 + i * dx, xb = xa + dx;
                const float za = z0 + j * dz, zb = za + dz;
                glVertex3f(xa, 0.0f, za);
                glVertex3f(xa, 0.0f, zb);
                glVertex3f(xb, 0.0f, zb);
                glVertex3f(xb, 0.0f, za);
            }
        }
    glEnd();
}

// =============================================================================
//  SECTION 4 — WORLD LAYOUT
//
//  Right-handed coordinate system, Y is up, 1 unit = 1 metre.
//  The room is centred on the origin with its floor at y = 0:
//
//        -X  <-- LIVING AREA  |  DINING AREA -->  +X
//
//                 z = -4.5 (north wall)
//                 +---------------------+
//                 |   TV     |  chairs  |
//      x = -6     | sofa/rug |  table   |     x = +6
//                 |          |  chairs  |
//                 +---------------------+
//                 z = +4.5 (south wall)
//
//  Every object below is placed by name; no call site repeats a raw literal.
// =============================================================================

// --- Room shell -------------------------------------------------------------
static const float ROOM_W      = 12.0f;                 // extent along X
static const float ROOM_H      =  3.2f;                 // floor-to-ceiling
static const float ROOM_D      =  9.0f;                 // extent along Z
static const float ROOM_HALF_W = ROOM_W * 0.5f;         // +/- 6.0
static const float ROOM_HALF_D = ROOM_D * 0.5f;         // +/- 4.5
static const int   WALL_GRID   = 24;                    // per-vertex lighting res
static const float SURFACE_EPS = 0.01f;                 // anti z-fighting offset

// --- Living area (negative X half) ------------------------------------------
// The sofa faces -X so that it looks straight at the wall-mounted TV, with the
// coffee table between them and the rug spanning the whole grouping.
static const float SOFA_X      = -1.9f, SOFA_Z = -1.0f;
static const float SOFA_FACING = 90.0f;                 // deg about Y: -Z -> -X
static const float COFFEE_X    = -3.5f, COFFEE_Z = -1.0f;
static const float TV_X        = -ROOM_HALF_W + SURFACE_EPS;   // on the -X wall
static const float TV_Y        =  1.75f, TV_Z = -1.0f;
static const float RUG_X       = -3.1f, RUG_Z = -1.0f;
static const float RUG_W       =  5.2f, RUG_D =  3.6f;

// --- Dining area (positive X half) ------------------------------------------
static const float DINE_X      =  3.4f, DINE_Z = 0.0f;
static const float DTABLE_TOP_W = 2.0f;                 // along X
static const float DTABLE_TOP_D = 1.2f;                 // along Z
static const float DTABLE_H     = 0.74f;                // floor to table top
static const float CHAIR_OFF_X  = 1.45f;                // East / West seats
static const float CHAIR_OFF_Z  = 1.05f;                // North / South seats

// --- Ceiling fixtures -------------------------------------------------------
static const float FAN_X = 0.0f, FAN_Z = 0.0f;          // centre of the room
static const float CHAND_X = DINE_X, CHAND_Z = DINE_Z;  // over the dining table
static const float CHAND_Y = 2.55f;                     // bulb height
static const float SPOT_X = COFFEE_X, SPOT_Z = COFFEE_Z;// living-area accent
// A downlight recessed INTO the ceiling slab: the emitter sits just above the
// ceiling plane, its visible fixture just below it. The 6 cm offset is
// deliberate and must not be reduced - see the GL_LIGHT1 note in drawRoom().
// Keeping the emitter strictly above every polygon in the room means no vertex
// is ever behind the spot, which is the case that breaks fixed-function
// lighting on some drivers.
static const float SPOT_Y        = ROOM_H + 0.06f;   // the light itself
static const float SPOT_MARKER_Y = ROOM_H - 0.07f;   // the visible fixture

// --- Wall pictures ----------------------------------------------------------
static const float PIC1_X = -3.0f;                      // on the north wall
static const float PIC2_X =  3.0f;                      // on the north wall
static const float PIC_Y  =  1.95f;
static const float PIC_Z  = -ROOM_HALF_D + SURFACE_EPS;

// =============================================================================
//  SECTION 5 — ROOM SHELL
//  Floor, ceiling and four walls. Each surface's normal points INTO the room so
//  that the interior is the lit side.
// =============================================================================
static void drawRoom()
{
    // --- Floor: plane already lies in XZ, normal straight up ---------------
    glPushMatrix();
      setMaterial(MAT_FLOOR);
      drawPlaneGrid(ROOM_W, ROOM_D, WALL_GRID, 0.0f, 1.0f, 0.0f);
    glPopMatrix();

    // --- Ceiling: same plane raised to ROOM_H, normal flipped downward -----
    //
    // GL_LIGHT1 is switched off for this one surface. It is a SPOT light aimed
    // straight down from just below the ceiling, so the ceiling is the only
    // geometry in the room sitting BEHIND it. A downward spot cannot illuminate
    // the ceiling anyway (it is outside the cone by construction), and leaving
    // the light enabled here is actively harmful: for a vertex behind a spot
    // whose normal is parallel to the spot axis, the fixed-function lighting
    // path on some drivers (Apple's GL among them) yields a NaN vertex colour,
    // which rasterises as pure black and wipes out even GL_EMISSION.
    // Disabling the light for this draw keeps the ceiling correctly lit by the
    // chandelier alone.
    const bool light1WasOn = light1On;
    if (light1WasOn) glDisable(GL_LIGHT1);
    glPushMatrix();
      glTranslatef(0.0f, ROOM_H, 0.0f);
      setMaterial(MAT_CEILING);
      drawPlaneGrid(ROOM_W, ROOM_D, WALL_GRID, 0.0f, -1.0f, 0.0f);
    glPopMatrix();
    if (light1WasOn) glEnable(GL_LIGHT1);

    setMaterial(MAT_WALL);

    // --- North wall (-Z), rotated up so its normal points to +Z ------------
    glPushMatrix();
      glTranslatef(0.0f, ROOM_H * 0.5f, -ROOM_HALF_D);
      glRotatef(90.0f, 1.0f, 0.0f, 0.0f);   // local +Y normal -> world +Z
      drawPlaneGrid(ROOM_W, ROOM_H, WALL_GRID, 0.0f, 1.0f, 0.0f);
    glPopMatrix();

    // --- South wall (+Z), normal points to -Z ------------------------------
    glPushMatrix();
      glTranslatef(0.0f, ROOM_H * 0.5f, ROOM_HALF_D);
      glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);  // local +Y normal -> world -Z
      drawPlaneGrid(ROOM_W, ROOM_H, WALL_GRID, 0.0f, 1.0f, 0.0f);
    glPopMatrix();

    // --- West wall (-X), normal points to +X (this is the TV wall) ---------
    glPushMatrix();
      glTranslatef(-ROOM_HALF_W, ROOM_H * 0.5f, 0.0f);
      glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);  // local +Y normal -> world +X
      drawPlaneGrid(ROOM_H, ROOM_D, WALL_GRID, 0.0f, 1.0f, 0.0f);
    glPopMatrix();

    // --- East wall (+X), normal points to -X -------------------------------
    glPushMatrix();
      glTranslatef(ROOM_HALF_W, ROOM_H * 0.5f, 0.0f);
      glRotatef(90.0f, 0.0f, 0.0f, 1.0f);   // local +Y normal -> world -X
      drawPlaneGrid(ROOM_H, ROOM_D, WALL_GRID, 0.0f, 1.0f, 0.0f);
    glPopMatrix();
}

// =============================================================================
//  SECTION 6 — CAMERA  (free-fly, first person)
//
//  State is a position plus two Euler angles. Each frame the forward vector is
//  rebuilt from them and handed to gluLookAt as eye + forward:
//
//      forward = ( cos(pitch) * sin(yaw),
//                  sin(pitch),
//                 -cos(pitch) * cos(yaw) )
//
//  yaw = 0 therefore looks down -Z, and increasing yaw swings to the right.
//  right = normalize(cross(forward, worldUp)) is used for strafing, and the
//  world up axis (never the camera's own up) is used for Q/E so vertical
//  movement stays vertical no matter where the camera is pointing.
// =============================================================================
static Vec3  camPos(4.6f, 1.65f, 3.9f);   // start inside, near the dining area
static float camYaw   = -35.0f;           // degrees, looking back at the room
static float camPitch =  -6.0f;           // degrees, slightly downward
static const float MOVE_SPEED = 0.28f;    // metres per key press
static const float MOUSE_SENS = 0.16f;    // degrees per pixel of mouse movement
static const float PITCH_LIMIT = 89.0f;   // clamp so the view can never flip

static const float DEG2RAD = 3.14159265358979f / 180.0f;

static Vec3 cameraForward()
{
    const float cy = cosf(camYaw * DEG2RAD),   sy = sinf(camYaw * DEG2RAD);
    const float cp = cosf(camPitch * DEG2RAD), sp = sinf(camPitch * DEG2RAD);
    return normalize(Vec3(cp * sy, sp, -cp * cy));
}

static Vec3 cameraRight()
{
    return normalize(cross(cameraForward(), Vec3(0.0f, 1.0f, 0.0f)));
}

// Build the view matrix for this frame.
static void applyCamera()
{
    const Vec3 f      = cameraForward();
    const Vec3 target = add(camPos, f);       // look one unit ahead of the eye
    gluLookAt(camPos.x, camPos.y, camPos.z,
              target.x, target.y, target.z,
              0.0f,     1.0f,     0.0f);      // world up
}

// =============================================================================
//  GLUT CALLBACKS
// =============================================================================

// Rebuild the perspective projection whenever the window changes size, so the
// aspect ratio always matches the viewport and nothing looks stretched.
void reshape(int w, int h)
{
    if (h == 0) h = 1;                      // guard against divide-by-zero
    winWidth  = w;
    winHeight = h;

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0,                        // vertical field of view
                   (double)w / (double)h,       // aspect ratio
                   0.1,                         // near plane
                   100.0);                      // far plane
    glMatrixMode(GL_MODELVIEW);
}

// ---------------------------------------------------------------------------
//  Animation timing constants (see timer() for the full explanation).
// ---------------------------------------------------------------------------
static const int   TIMER_MS         = 16;     // ~60 Hz
static const float FAN_DEG_PER_TICK = 6.0f;   // -> ~1 revolution per second


// =============================================================================
//  SECTION 7 — LIGHTING
//
//  GL_LIGHT0 : the dining chandelier. A positional (w = 1) white-ish lamp that
//              carries most of the room's ambient + diffuse fill.
//  GL_LIGHT1 : a warm accent SPOT light over the living-area coffee table,
//              with a cone cutoff and distance attenuation so it reads as a
//              focused pool rather than general illumination.
//
//  Both are re-positioned every frame AFTER gluLookAt. That ordering is the
//  whole trick: glLightfv(GL_POSITION) transforms the position by the current
//  modelview matrix, so specifying it after the view transform pins the lamp to
//  the room. Specifying it before would leave it fixed to the camera instead.
// =============================================================================
static const GLfloat L0_POS[]  = { CHAND_X, CHAND_Y, CHAND_Z, 1.0f };  // positional
static const GLfloat L0_AMB[]  = { 0.24f, 0.235f, 0.22f, 1.0f };
static const GLfloat L0_DIF[]  = { 0.95f, 0.92f, 0.86f, 1.0f };
static const GLfloat L0_SPEC[] = { 0.75f, 0.74f, 0.70f, 1.0f };

static const GLfloat L1_POS[]  = { SPOT_X, SPOT_Y, SPOT_Z, 1.0f };
static const GLfloat L1_DIR[]  = { 0.0f, -1.0f, 0.0f };                // straight down
static const GLfloat L1_AMB[]  = { 0.05f, 0.04f, 0.02f, 1.0f };
static const GLfloat L1_DIF[]  = { 1.00f, 0.78f, 0.45f, 1.0f };        // warm tungsten
static const GLfloat L1_SPEC[] = { 1.00f, 0.90f, 0.72f, 1.0f };

static void initLights()
{
    // A deliberately tiny global ambient: with both lamps off the room goes
    // almost black (but not pure black, so the scene is still intelligible).
    const GLfloat globalAmb[] = { 0.05f, 0.05f, 0.06f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);  // truer specular angles

    // --- Light 0: chandelier -------------------------------------------
    glLightfv(GL_LIGHT0, GL_AMBIENT,  L0_AMB);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  L0_DIF);
    glLightfv(GL_LIGHT0, GL_SPECULAR, L0_SPEC);
    glLightf (GL_LIGHT0, GL_CONSTANT_ATTENUATION,  1.00f);
    glLightf (GL_LIGHT0, GL_LINEAR_ATTENUATION,    0.030f);
    glLightf (GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.006f);

    // --- Light 1: warm spot --------------------------------------------
    glLightfv(GL_LIGHT1, GL_AMBIENT,  L1_AMB);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  L1_DIF);
    glLightfv(GL_LIGHT1, GL_SPECULAR, L1_SPEC);
    glLightf (GL_LIGHT1, GL_SPOT_CUTOFF,   35.0f);   // half-angle of the cone
    glLightf (GL_LIGHT1, GL_SPOT_EXPONENT, 12.0f);   // falloff toward the rim
    // Mild attenuation only: the lamp is mounted at ceiling height, so heavy
    // falloff would leave nothing but a faint smudge by the time it reaches the
    // floor. The cone cutoff above is what makes this light read as "focused".
    glLightf (GL_LIGHT1, GL_CONSTANT_ATTENUATION,  1.00f);
    glLightf (GL_LIGHT1, GL_LINEAR_ATTENUATION,    0.025f);
    glLightf (GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.006f);
}

// Called every frame, immediately after the camera has been applied.
static void placeLights()
{
    glLightfv(GL_LIGHT0, GL_POSITION, L0_POS);
    glLightfv(GL_LIGHT1, GL_POSITION, L1_POS);
    glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, L1_DIR);

    if (light0On) glEnable(GL_LIGHT0); else glDisable(GL_LIGHT0);
    if (light1On) glEnable(GL_LIGHT1); else glDisable(GL_LIGHT1);
}

// A small sphere at a lamp position, made to glow with GL_EMISSION when that
// lamp is on and left nearly black when it is off. Emission is restored to zero
// by the next setMaterial() call, so the glow cannot leak onto other objects.
static void drawLightMarker(float x, float y, float z,
                            const GLfloat colour[4], bool on)
{
    Material m = MAT_DARK_PLASTIC;
    for (int i = 0; i < 3; ++i) {
        m.emi[i] = on ? colour[i] : 0.02f;              // the glow itself
        m.dif[i] = on ? colour[i] * 0.5f : 0.05f;
    }
    m.emi[3] = 1.0f;

    glPushMatrix();
      glTranslatef(x, y, z);
      setMaterial(m);
      glutSolidSphere(0.085, 18, 14);
    glPopMatrix();
}

// =============================================================================
//  SECTION 8 — LIVING AREA FURNITURE
//
//  Modelling convention for every helper below: the object is built in its own
//  local frame with the origin on the FLOOR at the object's centre, facing -Z.
//  The helper never translates itself into the room - drawScene() does that -
//  so each helper can be reasoned about (and re-used) independently.
// =============================================================================

// --- Sofa ---------------------------------------------------------------
// Base + backrest + two armrests + three separated seat cushions.
static const float SOFA_W       = 2.30f;    // total width (local X)
static const float SOFA_D       = 0.95f;    // total depth (local Z)
static const float SOFA_BASE_H  = 0.38f;    // seat platform height
static const float SOFA_ARM_W   = 0.20f;
static const float SOFA_BACK_H  = 0.62f;    // backrest height above the base

static void drawSofa()
{
    const float innerW = SOFA_W - 2.0f * SOFA_ARM_W;   // width between armrests

    // Base platform: sits on the floor, centred on the local origin.
    setMaterial(MAT_FABRIC);
    glPushMatrix();
      glTranslatef(0.0f, SOFA_BASE_H * 0.5f, 0.0f);
      drawBox(SOFA_W, SOFA_BASE_H, SOFA_D);
    glPopMatrix();

    // Backrest: stands at the +Z edge, i.e. BEHIND a person sitting on the
    // sofa, so the sofa as a whole faces -Z as the convention above states.
    glPushMatrix();
      glTranslatef(0.0f,
                   SOFA_BASE_H + SOFA_BACK_H * 0.5f,
                   SOFA_D * 0.5f - 0.11f);
      drawBox(SOFA_W, SOFA_BACK_H, 0.22f);
    glPopMatrix();

    // Armrests: one at each end, raised above the seat.
    for (int side = -1; side <= 1; side += 2) {
        glPushMatrix();
          glTranslatef(side * (SOFA_W - SOFA_ARM_W) * 0.5f,
                       SOFA_BASE_H + 0.13f, -0.03f);
          drawBox(SOFA_ARM_W, 0.26f, SOFA_D - 0.06f);
        glPopMatrix();
    }

    // Three seat cushions, drawn with a slightly lighter fabric and small gaps
    // between them so they read as separate cushions rather than one slab.
    setMaterial(MAT_FABRIC_CUSHION);
    const float cushionW = innerW / 3.0f - 0.04f;
    for (int i = -1; i <= 1; ++i) {
        glPushMatrix();
          glTranslatef(i * (innerW / 3.0f), SOFA_BASE_H + 0.06f, -0.06f);
          drawBox(cushionW, 0.12f, SOFA_D - 0.28f);
        glPopMatrix();
    }

    // Two back cushions resting against the backrest.
    for (int i = -1; i <= 1; i += 2) {
        glPushMatrix();
          glTranslatef(i * innerW * 0.25f,
                       SOFA_BASE_H + 0.26f,
                       SOFA_D * 0.5f - 0.28f);
          drawBox(innerW * 0.44f, 0.34f, 0.12f);
        glPopMatrix();
    }
}

// --- Coffee / tea table -------------------------------------------------
static const float COFFEE_W = 1.10f, COFFEE_D = 0.62f, COFFEE_H = 0.40f;
static const float COFFEE_TOP_T = 0.06f;    // tabletop thickness
static const float COFFEE_LEG   = 0.07f;    // leg cross-section

static void drawCoffeeTable()
{
    setMaterial(MAT_WOOD);

    // Tabletop, its underside at COFFEE_H - COFFEE_TOP_T.
    glPushMatrix();
      glTranslatef(0.0f, COFFEE_H - COFFEE_TOP_T * 0.5f, 0.0f);
      drawBox(COFFEE_W, COFFEE_TOP_T, COFFEE_D);
    glPopMatrix();

    // Four legs, inset from the corners of the top.
    const float legH = COFFEE_H - COFFEE_TOP_T;
    const float ox   = COFFEE_W * 0.5f - COFFEE_LEG - 0.03f;
    const float oz   = COFFEE_D * 0.5f - COFFEE_LEG - 0.03f;
    setMaterial(MAT_WOOD_DARK);
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
            glPushMatrix();
              glTranslatef(sx * ox, legH * 0.5f, sz * oz);
              drawBox(COFFEE_LEG, legH, COFFEE_LEG);
            glPopMatrix();
        }
}

// --- Wall-mounted flat-screen TV ----------------------------------------
//
// The display surface is generated procedurally at start-up (see
// initScreenTexture) so the program stays entirely self-contained: there is no
// image file to ship alongside main.cpp. Set SCREEN_TEXTURE to 0 to fall back
// to a flat contrasting emissive panel instead.
#define SCREEN_TEXTURE 1

static const float TV_W = 1.60f, TV_H = 0.94f;   // outer bezel size
static const float TV_BEZEL = 0.05f;             // bezel border width
static GLuint screenTex = 0;

#if SCREEN_TEXTURE
// Build a 64x64 RGB image in code: a colour-graded gradient overlaid with a
// checker, which reads clearly as "something is on the screen" from any angle.
static void initScreenTexture()
{
    const int N = 64;
    static unsigned char px[64 * 64 * 3];
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            const int   i     = (y * N + x) * 3;
            const bool  check = ((x / 8) + (y / 8)) % 2 == 0;
            const float u     = (float)x / (N - 1);
            const float v     = (float)y / (N - 1);
            const float shade = check ? 1.0f : 0.55f;
            px[i + 0] = (unsigned char)(255 * shade * (0.15f + 0.55f * u));
            px[i + 1] = (unsigned char)(255 * shade * (0.35f + 0.45f * v));
            px[i + 2] = (unsigned char)(255 * shade * (0.75f - 0.35f * u));
        }
    }
    glGenTextures(1, &screenTex);
    glBindTexture(GL_TEXTURE_2D, screenTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, N, N, 0, GL_RGB, GL_UNSIGNED_BYTE, px);
    glBindTexture(GL_TEXTURE_2D, 0);
}
#else
static void initScreenTexture() {}
#endif

// Modelled in a local frame whose origin is the point where the mount meets the
// wall, with the screen facing +X (the TV hangs on the west wall).
static void drawTV()
{
    // Bracket holding the panel off the wall.
    setMaterial(MAT_METAL);
    glPushMatrix();
      glTranslatef(0.05f, 0.0f, 0.0f);
      drawBox(0.10f, 0.34f, 0.22f);
    glPopMatrix();

    // Bezel: a shallow dark slab. Rotated so its own +Z front face looks along
    // world +X, out into the room.
    glPushMatrix();
      glTranslatef(0.13f, 0.0f, 0.0f);
      glRotatef(90.0f, 0.0f, 1.0f, 0.0f);       // panel front (+Z) -> +X
      setMaterial(MAT_DARK_PLASTIC);
      drawBox(TV_W, TV_H, 0.07f);

      // Screen: inset inside the bezel and pushed a hair proud of it so it
      // cannot z-fight with the bezel face it sits on.
      const float sw = TV_W - 2.0f * TV_BEZEL;
      const float sh = TV_H - 2.0f * TV_BEZEL;
#if SCREEN_TEXTURE
      // A little emission stands in for the backlight, so the panel reads as a
      // powered screen even on the dim TV wall. GL_MODULATE leaves emission
      // untouched, so the generated image still shows through it.
      Material screen = MAT_GLASS;
      screen.emi[0] = 0.10f; screen.emi[1] = 0.11f; screen.emi[2] = 0.13f;
      setMaterial(screen);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, screenTex);
      // Modulate keeps the lighting response (and the glossy highlight) while
      // showing the generated image.
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-sw * 0.5f, -sh * 0.5f, 0.037f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( sw * 0.5f, -sh * 0.5f, 0.037f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( sw * 0.5f,  sh * 0.5f, 0.037f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-sw * 0.5f,  sh * 0.5f, 0.037f);
      glEnd();
      glBindTexture(GL_TEXTURE_2D, 0);
      glDisable(GL_TEXTURE_2D);         // never leave texturing on for the scene
#else
      setMaterial(MAT_GLASS);
      Material lit = MAT_GLASS;         // fallback: flat contrasting panel
      lit.emi[0] = 0.10f; lit.emi[1] = 0.22f; lit.emi[2] = 0.34f;
      setMaterial(lit);
      glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f(-sw * 0.5f, -sh * 0.5f, 0.037f);
        glVertex3f( sw * 0.5f, -sh * 0.5f, 0.037f);
        glVertex3f( sw * 0.5f,  sh * 0.5f, 0.037f);
        glVertex3f(-sw * 0.5f,  sh * 0.5f, 0.037f);
      glEnd();
#endif
    glPopMatrix();
}

// --- Rug ----------------------------------------------------------------
// A very thin slab rather than a flat quad: giving it real thickness and
// lifting it by SURFACE_EPS keeps it from z-fighting with the floor.
static void drawRug()
{
    setMaterial(MAT_RUG);
    glPushMatrix();
      glTranslatef(0.0f, SURFACE_EPS, 0.0f);
      drawBox(RUG_W, 0.012f, RUG_D);
    glPopMatrix();
}

// =============================================================================
//  SECTION 9 — DINING AREA FURNITURE
// =============================================================================

// --- Dining table -------------------------------------------------------
static const float DTABLE_TOP_T = 0.07f;    // top thickness
static const float DTABLE_LEG   = 0.09f;    // leg cross-section

static void drawDiningTable()
{
    setMaterial(MAT_WOOD);

    // Rectangular top, its upper surface exactly at DTABLE_H.
    glPushMatrix();
      glTranslatef(0.0f, DTABLE_H - DTABLE_TOP_T * 0.5f, 0.0f);
      drawBox(DTABLE_TOP_W, DTABLE_TOP_T, DTABLE_TOP_D);
    glPopMatrix();

    // Exactly four legs, inset from the corners so they read as legs rather
    // than as a solid box.
    setMaterial(MAT_WOOD_DARK);
    const float legH = DTABLE_H - DTABLE_TOP_T;
    const float ox   = DTABLE_TOP_W * 0.5f - DTABLE_LEG - 0.06f;
    const float oz   = DTABLE_TOP_D * 0.5f - DTABLE_LEG - 0.06f;
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
            glPushMatrix();
              glTranslatef(sx * ox, legH * 0.5f, sz * oz);
              drawBox(DTABLE_LEG, legH, DTABLE_LEG);
            glPopMatrix();
        }
}

// --- Dining chair -------------------------------------------------------
// Canonical local frame: origin on the floor at the chair's centre, seat facing
// -Z, backrest therefore on the +Z side (behind the sitter). drawScene() rotates
// each chair so that -Z points at the table.
static const float CHAIR_W      = 0.46f;
static const float CHAIR_D      = 0.46f;
static const float CHAIR_SEAT_H = 0.45f;
static const float CHAIR_LEG    = 0.05f;
static const float CHAIR_BACK_H = 0.52f;

static void drawChair()
{
    // Seat slab.
    setMaterial(MAT_WOOD);
    glPushMatrix();
      glTranslatef(0.0f, CHAIR_SEAT_H, 0.0f);
      drawBox(CHAIR_W, 0.06f, CHAIR_D);
    glPopMatrix();

    // Backrest: a vertical panel on the +Z edge, raised above the seat, plus
    // two uprights connecting it down to the seat.
    glPushMatrix();
      glTranslatef(0.0f,
                   CHAIR_SEAT_H + CHAIR_BACK_H * 0.5f + 0.03f,
                   CHAIR_D * 0.5f - 0.03f);
      drawBox(CHAIR_W, CHAIR_BACK_H, 0.05f);
    glPopMatrix();

    // Four legs, one under each corner of the seat.
    setMaterial(MAT_WOOD_DARK);
    const float ox = CHAIR_W * 0.5f - CHAIR_LEG * 0.5f - 0.02f;
    const float oz = CHAIR_D * 0.5f - CHAIR_LEG * 0.5f - 0.02f;
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
            glPushMatrix();
              glTranslatef(sx * ox, (CHAIR_SEAT_H - 0.03f) * 0.5f, sz * oz);
              drawBox(CHAIR_LEG, CHAIR_SEAT_H - 0.03f, CHAIR_LEG);
            glPopMatrix();
        }
}

// =============================================================================
//  SECTION 10 — CEILING FAN
//
//  Hierarchy, and the reason the matrix stack matters here:
//
//      [fan origin at the ceiling]
//         |- down-rod        (static)
//         |- motor housing   (static)
//         `- BLADE GROUP     <- glRotatef(fanAngle) applied to this group only
//              |- blade 1 ... blade 4
//
//  The rotation lives inside its own push/pop, so it spins the blades without
//  moving the housing and without leaking into anything drawn afterwards.
// =============================================================================
static const float FAN_ROD_H    = 0.22f;    // drop from the ceiling
static const float FAN_HUB_R    = 0.13f;
static const float FAN_HUB_H    = 0.10f;
static const float FAN_BLADE_L  = 0.85f;
static const float FAN_BLADE_W  = 0.17f;
static const int   FAN_BLADES   = 4;

static void drawCeilingFan()
{
    setMaterial(MAT_METAL);

    // Down-rod from the ceiling to the motor.
    glPushMatrix();
      glTranslatef(0.0f, -FAN_ROD_H * 0.5f, 0.0f);
      drawBox(0.05f, FAN_ROD_H, 0.05f);
    glPopMatrix();

    // Motor housing: a GLU cylinder. gluCylinder is built along +Z, so it is
    // rotated onto the Y axis. (glutSolidCylinder is freeglut-only and would
    // not compile against Apple's GLUT.)
    GLUquadric* q = gluNewQuadric();
    gluQuadricNormals(q, GLU_SMOOTH);       // generated normals, so it shades
    glPushMatrix();
      glTranslatef(0.0f, -FAN_ROD_H - FAN_HUB_H, 0.0f);
      glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);  // cylinder axis +Z -> +Y
      gluCylinder(q, FAN_HUB_R, FAN_HUB_R, FAN_HUB_H, 24, 1);
      gluDisk(q, 0.0, FAN_HUB_R, 24, 1);    // cap the bottom
      glPushMatrix();
        glTranslatef(0.0f, 0.0f, FAN_HUB_H);
        gluDisk(q, 0.0, FAN_HUB_R, 24, 1);  // cap the top
      glPopMatrix();
    glPopMatrix();
    gluDeleteQuadric(q);

    // ---- Rotating blade group ------------------------------------------
    glPushMatrix();
      glTranslatef(0.0f, -FAN_ROD_H - FAN_HUB_H * 0.5f, 0.0f);
      glRotatef(fanAngle, 0.0f, 1.0f, 0.0f);        // <-- the animation
      for (int i = 0; i < FAN_BLADES; ++i) {
          glPushMatrix();
            glRotatef(i * (360.0f / FAN_BLADES), 0.0f, 1.0f, 0.0f);
            glTranslatef(FAN_HUB_R + FAN_BLADE_L * 0.5f, 0.0f, 0.0f);
            glRotatef(9.0f, 1.0f, 0.0f, 0.0f);      // slight pitch on the blade
            drawBox(FAN_BLADE_L, 0.02f, FAN_BLADE_W);
          glPopMatrix();
      }
    glPopMatrix();
}

// =============================================================================
//  SECTION 11 — DECORATIVE FIXTURES
// =============================================================================

// --- Framed wall picture ------------------------------------------------
// Modelled in the XY plane facing +Z; the caller rotates it onto its wall.
static void drawFramedPicture(float w, float h, const Material& art)
{
    setMaterial(MAT_WOOD);
    glPushMatrix();                         // outer frame
      drawBox(w, h, 0.04f);
    glPopMatrix();

    setMaterial(art);                       // inner picture surface, proud of it
    glPushMatrix();
      glTranslatef(0.0f, 0.0f, 0.021f);
      drawBox(w - 0.10f, h - 0.10f, 0.005f);
    glPopMatrix();
}

// --- Chandelier ---------------------------------------------------------
// Hangs over the dining table at the GL_LIGHT0 position: a rod, a central body
// and a ring of arms ending in small bulbs.
static void drawChandelier()
{
    setMaterial(MAT_METAL);

    const float rodLen = ROOM_H - CHAND_Y - 0.12f;
    glPushMatrix();                         // suspension rod
      glTranslatef(0.0f, ROOM_H - rodLen * 0.5f, 0.0f);
      drawBox(0.035f, rodLen, 0.035f);
    glPopMatrix();

    glPushMatrix();                         // central body
      glTranslatef(0.0f, CHAND_Y + 0.10f, 0.0f);
      glutSolidSphere(0.11, 18, 14);
    glPopMatrix();

    // Three arms with bulbs. The bulbs glow only while Light 0 is on, matching
    // the lamp they represent.
    Material bulb = MAT_DARK_PLASTIC;
    for (int i = 0; i < 3; ++i) {
        const float a = i * 120.0f;
        glPushMatrix();
          glRotatef(a, 0.0f, 1.0f, 0.0f);
          setMaterial(MAT_METAL);
          glPushMatrix();                   // arm
            glTranslatef(0.16f, CHAND_Y + 0.08f, 0.0f);
            glRotatef(20.0f, 0.0f, 0.0f, 1.0f);
            drawBox(0.28f, 0.025f, 0.025f);
          glPopMatrix();
          for (int k = 0; k < 3; ++k) {
              bulb.emi[k] = light0On ? L0_DIF[k] * 0.85f : 0.02f;
              bulb.dif[k] = light0On ? L0_DIF[k] * 0.4f  : 0.06f;
          }
          setMaterial(bulb);
          glPushMatrix();                   // bulb at the arm's end
            glTranslatef(0.30f, CHAND_Y + 0.13f, 0.0f);
            glutSolidSphere(0.055, 14, 10);
          glPopMatrix();
        glPopMatrix();
    }
}

// --- Recessed spot fixture ----------------------------------------------
// The visible housing for GL_LIGHT1, sunk into the ceiling above the rug.
static void drawSpotFixture()
{
    setMaterial(MAT_METAL);
    glPushMatrix();
      glTranslatef(0.0f, ROOM_H - 0.04f, 0.0f);
      drawBox(0.26f, 0.08f, 0.26f);
    glPopMatrix();
}

// =============================================================================
//  SECTION 12 — SCENE ASSEMBLY
//  Each object is placed inside its own glPushMatrix/glPopMatrix pair, so its
//  transform cannot affect anything drawn afterwards.
// =============================================================================
static void drawScene()
{
    drawRoom();

    // --- Living area -----------------------------------------------------
    // The rug goes down first, then the furniture standing on it.
    glPushMatrix();
      glTranslatef(RUG_X, 0.0f, RUG_Z);
      drawRug();
    glPopMatrix();

    // The sofa is rotated to face the TV wall (-X). SOFA_FACING converts the
    // helper's canonical -Z heading into that direction.
    glPushMatrix();
      glTranslatef(SOFA_X, 0.0f, SOFA_Z);
      glRotatef(SOFA_FACING, 0.0f, 1.0f, 0.0f);
      drawSofa();
    glPopMatrix();

    glPushMatrix();
      glTranslatef(COFFEE_X, 0.0f, COFFEE_Z);
      glRotatef(SOFA_FACING, 0.0f, 1.0f, 0.0f);   // aligned with the sofa
      drawCoffeeTable();
    glPopMatrix();

    // TV hangs on the west wall, facing back into the room (+X).
    glPushMatrix();
      glTranslatef(TV_X, TV_Y, TV_Z);
      drawTV();
    glPopMatrix();

    // --- Dining area -----------------------------------------------------
    glPushMatrix();
      glTranslatef(DINE_X, 0.0f, DINE_Z);
      drawDiningTable();
    glPopMatrix();

    // Four chairs, one per side. Each chair is modelled facing -Z, so the
    // rotation below turns that heading toward the table:
    //   North seat (-Z side) must face +Z -> 180 deg
    //   South seat (+Z side) must face -Z ->   0 deg
    //   East  seat (+X side) must face -X ->  90 deg
    //   West  seat (-X side) must face +X -> -90 deg
    const float chairDX[4]  = { 0.0f, 0.0f,  CHAIR_OFF_X, -CHAIR_OFF_X };
    const float chairDZ[4]  = { -CHAIR_OFF_Z, CHAIR_OFF_Z, 0.0f, 0.0f };
    const float chairRot[4] = { 180.0f, 0.0f, 90.0f, -90.0f };
    for (int i = 0; i < 4; ++i) {
        glPushMatrix();
          glTranslatef(DINE_X + chairDX[i], 0.0f, DINE_Z + chairDZ[i]);
          glRotatef(chairRot[i], 0.0f, 1.0f, 0.0f);
          drawChair();
        glPopMatrix();
    }

    // --- Ceiling fixtures -------------------------------------------------
    glPushMatrix();
      glTranslatef(FAN_X, ROOM_H, FAN_Z);
      drawCeilingFan();
    glPopMatrix();

    glPushMatrix();
      glTranslatef(CHAND_X, 0.0f, CHAND_Z);   // chandelier uses absolute heights
      drawChandelier();
    glPopMatrix();

    glPushMatrix();
      glTranslatef(SPOT_X, 0.0f, SPOT_Z);
      drawSpotFixture();
    glPopMatrix();

    // --- Framed pictures on the north wall --------------------------------
    Material art1 = MAT_FABRIC;  art1.dif[0]=0.20f; art1.dif[1]=0.42f; art1.dif[2]=0.30f;
    Material art2 = MAT_FABRIC;  art2.dif[0]=0.46f; art2.dif[1]=0.34f; art2.dif[2]=0.16f;
    glPushMatrix();
      glTranslatef(PIC1_X, PIC_Y, PIC_Z);
      drawFramedPicture(0.85f, 0.62f, art1);
    glPopMatrix();
    glPushMatrix();
      glTranslatef(PIC2_X, PIC_Y, PIC_Z);
      drawFramedPicture(0.70f, 0.90f, art2);
    glPopMatrix();

    // Glowing markers showing exactly where each light source sits.
    drawLightMarker(L0_POS[0], L0_POS[1],     L0_POS[2], L0_DIF, light0On);
    drawLightMarker(L1_POS[0], SPOT_MARKER_Y, L1_POS[2], L1_DIF, light1On);
}

// =============================================================================
//  SECTION 13 — 2D HELP / STATUS OVERLAY
//
//  Drawn in a temporary orthographic projection with lighting and depth testing
//  switched off, so the text is always legible - including when both lamps are
//  off and the room itself is nearly black. All modified state is restored.
// =============================================================================
static void drawString(float x, float y, const char* s)
{
    glRasterPos2f(x, y);
    // glutBitmapString() is a freeglut extension and does not exist in Apple's
    // GLUT, so the string is emitted one character at a time.
    for (const char* p = s; *p; ++p)
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *p);
}

static void drawOverlay()
{
    // --- switch to a screen-space projection ------------------------------
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
      glLoadIdentity();
      gluOrtho2D(0.0, (GLdouble)winWidth, 0.0, (GLdouble)winHeight);
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
        glLoadIdentity();

        glDisable(GL_LIGHTING);         // text must not be shaded by the scene
        glDisable(GL_DEPTH_TEST);       // and must never be occluded by it

        char buf[128];
        const float top = (float)winHeight - 22.0f;

        glColor3f(1.0f, 1.0f, 1.0f);
        drawString(14.0f, top,         "3D Living + Dining Room");
        glColor3f(0.80f, 0.82f, 0.86f);
        drawString(14.0f, top - 20.0f, "WASD move   Q/E up-down   mouse or arrows look   Esc quit");

        // --- live state readout -------------------------------------------
        snprintf(buf, sizeof(buf), "[1] Light 0 chandelier : %s",
                 light0On ? "ON " : "OFF");
        glColor3f(light0On ? 1.0f : 0.45f, light0On ? 0.95f : 0.45f,
                  light0On ? 0.75f : 0.45f);
        drawString(14.0f, top - 44.0f, buf);

        snprintf(buf, sizeof(buf), "[2] Light 1 warm spot  : %s",
                 light1On ? "ON " : "OFF");
        glColor3f(light1On ? 1.0f : 0.45f, light1On ? 0.85f : 0.45f,
                  light1On ? 0.55f : 0.45f);
        drawString(14.0f, top - 62.0f, buf);

        snprintf(buf, sizeof(buf), "[F] Ceiling fan        : %s",
                 fanOn ? "ON " : "OFF");
        glColor3f(fanOn ? 0.75f : 0.45f, fanOn ? 0.90f : 0.45f,
                  fanOn ? 1.00f : 0.45f);
        drawString(14.0f, top - 80.0f, buf);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
      glPopMatrix();                    // restore the modelview
      glMatrixMode(GL_PROJECTION);
    glPopMatrix();                      // restore the perspective projection
    glMatrixMode(GL_MODELVIEW);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    applyCamera();          // view transform first: everything after is world space
    placeLights();          // ...so lamp positions land in world space, not eye space


    drawScene();


    drawOverlay();          // 2D text last, on top of the finished 3D frame


    glutSwapBuffers();
}

// -----------------------------------------------------------------------------
//  Keyboard: camera translation, light toggles, fan toggle, quit.
//  Every branch is case-insensitive, and any unbound key falls through with no
//  side effect at all.
// -----------------------------------------------------------------------------
void keyboard(unsigned char key, int, int)
{
    const Vec3 f = cameraForward();
    const Vec3 r = cameraRight();
    const Vec3 worldUp(0.0f, 1.0f, 0.0f);

    switch (key) {
        // --- translation ---------------------------------------------------
        case 'w': case 'W':                     // forward along the look vector
            camPos = add(camPos, scale(f, MOVE_SPEED)); break;
        case 's': case 'S':                     // backward
            camPos = add(camPos, scale(f, -MOVE_SPEED)); break;
        case 'a': case 'A':                     // strafe left
            camPos = add(camPos, scale(r, -MOVE_SPEED)); break;
        case 'd': case 'D':                     // strafe right
            camPos = add(camPos, scale(r, MOVE_SPEED)); break;
        case 'q': case 'Q':                     // straight up (world axis)
            camPos = add(camPos, scale(worldUp, MOVE_SPEED)); break;
        case 'e': case 'E':                     // straight down (world axis)
            camPos = add(camPos, scale(worldUp, -MOVE_SPEED)); break;

        // --- toggles --------------------------------------------------------
        case '1': light0On = !light0On; break;  // chandelier
        case '2': light1On = !light1On; break;  // warm spot
        case 'f': case 'F': fanOn = !fanOn; break;

        case 27: exit(0);                       // Esc quits

        default: return;                        // unbound key: do nothing at all
    }
    glutPostRedisplay();
}

// -----------------------------------------------------------------------------
//  Arrow keys: an alternative to the mouse for looking around, using exactly the
//  same yaw/pitch state.
// -----------------------------------------------------------------------------
void specialKeys(int key, int, int)
{
    const float step = 3.0f;                    // degrees per press
    switch (key) {
        case GLUT_KEY_LEFT:  camYaw   -= step; break;
        case GLUT_KEY_RIGHT: camYaw   += step; break;
        case GLUT_KEY_UP:    camPitch += step; break;
        case GLUT_KEY_DOWN:  camPitch -= step; break;
        default: return;
    }
    if (camPitch >  PITCH_LIMIT) camPitch =  PITCH_LIMIT;
    if (camPitch < -PITCH_LIMIT) camPitch = -PITCH_LIMIT;
    if (camYaw >= 360.0f) camYaw -= 360.0f;     // same wrap as the mouse path
    if (camYaw <    0.0f) camYaw += 360.0f;
    glutPostRedisplay();
}

// -----------------------------------------------------------------------------
//  Mouse look.
//
//  The pointer is warped back to the centre of the window after every motion
//  event, so the mouse can be moved indefinitely in one direction. That warp
//  itself generates another motion event, which would be read as user input and
//  fight the real movement - hence the ignoreNextMotion latch.
// -----------------------------------------------------------------------------
static bool ignoreNextMotion = true;            // also swallows the first event

void passiveMotion(int x, int y)
{
    if (ignoreNextMotion) {                     // this event came from our warp
        ignoreNextMotion = false;
        return;
    }

    const int cx = winWidth / 2, cy = winHeight / 2;
    const int dx = x - cx, dy = y - cy;
    if (dx == 0 && dy == 0) return;             // nothing moved: no warp needed

    camYaw   += dx * MOUSE_SENS;                // right on screen = yaw right
    camPitch -= dy * MOUSE_SENS;                // up on screen = pitch up

    // Clamp so the camera can never tip past vertical and flip the horizon.
    if (camPitch >  PITCH_LIMIT) camPitch =  PITCH_LIMIT;
    if (camPitch < -PITCH_LIMIT) camPitch = -PITCH_LIMIT;

    // Keep yaw in a sane range rather than letting it grow without bound.
    if (camYaw >= 360.0f) camYaw -= 360.0f;
    if (camYaw <    0.0f) camYaw += 360.0f;

    ignoreNextMotion = true;
    glutWarpPointer(cx, cy);
    glutPostRedisplay();
}

// -----------------------------------------------------------------------------
//  Animation timer.
//
//  Frame-rate independence comes from driving the angle off this fixed-interval
//  GLUT timer rather than off the display callback: however fast or slow the
//  machine renders, the timer is scheduled every TIMER_MS milliseconds, so the
//  fan completes a revolution in the same wall-clock time everywhere.
//
//      60 ticks/sec * 6 deg/tick = 360 deg/sec = one revolution per second
//
//  fmodf keeps the stored angle inside 0..360 instead of growing without bound
//  and slowly losing float precision.
// -----------------------------------------------------------------------------
void timer(int)
{
    glutTimerFunc(TIMER_MS, timer, 0);        // self re-registering
    if (fanOn)
        fanAngle = fmodf(fanAngle + FAN_DEG_PER_TICK, 360.0f);
    glutPostRedisplay();
}

// =============================================================================
//  INITIALISATION
// =============================================================================
void initGL()
{
    glEnable(GL_DEPTH_TEST);        // hidden surface removal
    glEnable(GL_LIGHTING);          // Phong lighting on
    glEnable(GL_NORMALIZE);         // keep normals unit-length under scaling
    glDisable(GL_COLOR_MATERIAL);   // materials come only from glMaterialfv
    glShadeModel(GL_SMOOTH);        // Gouraud interpolation across faces

    glClearColor(0.03f, 0.03f, 0.05f, 1.0f);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winWidth, winHeight);
    glutCreateWindow("3D Living + Dining Room  -  Computer Graphics");

    initGL();
    initLights();
    initScreenTexture();


    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutPassiveMotionFunc(passiveMotion);
    glutTimerFunc(TIMER_MS, timer, 0);

    glutMainLoop();
    return 0;
}
