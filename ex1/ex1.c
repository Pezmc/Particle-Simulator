////////////////////////////////////////////////////////////////
// Based on code provided for COMP37111 coursework, 2013-14
// by Arturs Bekasovs and Toby Howard
//
// Goal: Draws a particle emitter simulating a waterfall
// New Goal: Draw a cube with particles emmitting from every side
//
// Distance is in meters
// Speed is in meters per second
/////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#ifdef __APPLE__
  #include <glut.h> //GLUT/glut.h
  #include <OpenGL.h> //OpenGL/OpenGL.h
#else
  #include <GL/glut.h>
#endif

////////////// Define ///////////////

/* Useful information */
#define PI 3.1415926535897932384626433832795028841971
#define DEG_TO_RAD PI/180

/* How large is our emmitter array */
#define EMITTER_LIMIT 6

/* Particles/emiter limit */
#define PARTICLES_PER_EMITTER_LIMIT 1000

/* Define Some Keys */
#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3

/* Gravity default */
#define GRAVITY_STRENGTH -9.8

////////////// Structs //////////////

/*
 * A 3d position (or vector)
 */
typedef struct {
  GLfloat x, y, z;
} Vector;

/*
 * A particle (or voxel) in 3d space
 */
typedef struct {
  GLfloat r, g, b; // colour
  Vector position; // current position
  Vector velocity; //current velocity

  // Currently colliding with the floor?
  int yCollision;

  // "dead" = Not moving
  int dead;
  float deadTime;

  // Spawned yet?
  int firstSpawn;

} Particle;

/* particle struct */

typedef struct {
  Vector bottomLeft;
  Vector topRight;

  GLfloat r, g, b; // color

  Vector spawnVelocity;

  Particle particles[PARTICLES_PER_EMITTER_LIMIT];

  int numberOfParticles;
} SurfaceEmitter;

//////////// Globals //////////////

/* Rotate the scene around */
float cameraLoopYPosition = 0;
float cameraLoopYAngle = 45;

/* Have we received keyboard input yet? */
int rotationKeyboardInputReceived = 0;

/* Window size */
int WINDOW_WIDTH = 800;
int WINDOW_HEIGHT = 600;

/* Axis */
GLuint axisList;
int AXIS_SIZE = 100;
int axisEnabled = 1;

/* Floor grid */
int FLOOR_SIZE = 100;
GLuint gridFloorList;

/* Frame counting */
int frameCount = 0;
float fps = 0;

/* Time from open gl */
int currentTime = 0;

/* Time at which the last second happened */
int lastFPSUpdateTime = 0;

/* Last time the idle function was called */
int lastFrameTime = 0;

/* How many second each frame represents (usually less than 1) */
float deltaTime = 0.01;

/* How strong is gravity at the moment */
float gravityStrength = GRAVITY_STRENGTH;

/* Our array of emitters */
SurfaceEmitter emitters[6];
int emitterCount = 0;

///////////////// Helpers ////////////////////

/**
 * Return a random double between 0 and 1
 */
double randomNumber() {
  return (rand() / (double) RAND_MAX);
}

/**
 * Calculate the current FPS, updating
 * - frameCount
 * - lastFrameTime
 * - deltaTime
 * - currentTime
 */
void calculateFPS() {
  //  Increase frame count
  frameCount++;

  //  Get the number of milliseconds since glutInit called
  //  (or first call to glutGet(GLUT ELAPSED TIME)).
  currentTime = glutGet(GLUT_ELAPSED_TIME);

  //  Calculate time passed
  float timeInterval = currentTime - lastFPSUpdateTime;

  if (timeInterval > 1000) {
    //  calculate the number of frames per second
    fps = frameCount / (timeInterval / 1000.0f);

    //  Set time
    lastFPSUpdateTime = currentTime;

    //  Reset frame count
    frameCount = 0;
  }

  // Calculate time passed since last frame
  float timeSinceLastFrameMS = currentTime - lastFrameTime;

  // Delta time is used for physica
  deltaTime = timeSinceLastFrameMS / (1000.0f);

  lastFrameTime = currentTime;
}

/////////////// Main Draw ///////////////////

/* Draw all the particles on the screen */
void drawParticles() {
  // Draw each particle
  int i;
  glBegin(GL_POINTS);
  /*for (i= 0; i < numParticles; i++)  {
   glColor3f(particles[i].r, particles[i].g, particles[i].b); // color
   glVertex3f(particles[i].position.x, particles[i].position.y, particles[i].position.z); // position
   }*/
  glEnd();
}

////////////// Drawing Helpers ////////////////

/**
 * Takes a string and draws it "floating" at x-y
 * @param font GLUT bitmap font e.g. GLUT_BITMAP_HELVETICA_18
 * @param x Value between 0->1 representing x position
 * @param y Value between 0->1 representing y position
 * Abstracted: http://www.cs.manchester.ac.uk/ugt/COMP27112/OpenGL/frames.txt
 * Author: Toby Howard. toby@cs.man.ac.uk.
 */
void drawString(void *font, float x, float y, char *str) {
  char *ch; //temp pointer to store current char
  GLint matrixMode; //what mode are we in
  GLboolean lightingOn; //is lighting on?

  lightingOn = glIsEnabled(GL_LIGHTING); /* lighting on? */
  if (lightingOn)
    glDisable(GL_LIGHTING);

  //Borrowed from Toby's code
  glGetIntegerv(GL_MATRIX_MODE, &matrixMode); /* matrix mode atm? */

  glMatrixMode(GL_PROJECTION); //Projection mode
  glPushMatrix(); //New matrix
  glLoadIdentity();
  gluOrtho2D(0.0, 1.0, 0.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix(); //Again
  glLoadIdentity();
  glPushAttrib(GL_COLOR_BUFFER_BIT); /* save current colour */
  glColor3f(0.0, 0.0, 0.0); //set to black
  glRasterPos3f(x, y, 0.0); //where to draw?
  for (ch = str; *ch; ch++) { //for all chars...
    glutBitmapCharacter(font, (int) *ch);
  }
  glPopAttrib();
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(matrixMode);
  if (lightingOn)
    glEnable(GL_LIGHTING);
}

/**
 * Draw a cube out of 6 surfaces
 * Each with a unique color
 * @param size How large the cube should be in OpenGL units
 */
void drawCube(int size) {
  // Half the requested size
  size /= 2;

  // White side - BACK
  glBegin(GL_POLYGON);
  glColor3f(1.0, 1.0, 1.0);
  glVertex3f(size, -size, size);
  glVertex3f(size, size, size);
  glVertex3f(-size, size, size);
  glVertex3f(-size, -size, size);
  glEnd();

  // Purple side - RIGHT
  glBegin(GL_POLYGON);
  glColor3f(1.0, 0.0, 1.0);
  glVertex3f(size, -size, -size);
  glVertex3f(size, size, -size);
  glVertex3f(size, size, size);
  glVertex3f(size, -size, size);
  glEnd();

  // Green side - LEFT
  glBegin(GL_POLYGON);
  glColor3f(0.0, 1.0, 0.0);
  glVertex3f(-size, -size, size);
  glVertex3f(-size, size, size);
  glVertex3f(-size, size, -size);
  glVertex3f(-size, -size, -size);
  glEnd();

  // Blue side - TOP
  glBegin(GL_POLYGON);
  glColor3f(0.0, 0.0, 1.0);
  glVertex3f(size, size, size);
  glVertex3f(size, size, -size);
  glVertex3f(-size, size, -size);
  glVertex3f(-size, size, size);
  glEnd();

  // Red side - BOTTOM
  glBegin(GL_POLYGON);
  glColor3f(1.0, 0.0, 0.0);
  glVertex3f(size, -size, -size);
  glVertex3f(size, -size, size);
  glVertex3f(-size, -size, size);
  glVertex3f(-size, -size, -size);
  glEnd();

  // Black side - BACK
  glBegin(GL_POLYGON);
  glColor3f(0.0, 0.0, 0.0);
  glVertex3f(-size, size, -size);
  glVertex3f(-size, -size, -size);
  glVertex3f(size, -size, -size);
  glVertex3f(size, size, -size);
  glEnd();
}

/**
 * Draw the current FPS on the screen
 */
void drawFPS() {
  glLoadIdentity();

  char str[80];
  sprintf(str, "FPS: %4.2f", fps);

  //  Print the FPS to the window
  drawString(GLUT_BITMAP_HELVETICA_10, 0.9, 0.97, str);
}

////////////// Keyboard Handling ////////////////

/* Standard key press */
void keyboard(unsigned char key, int x, int y) {
  if (key == 27)
    exit(0);
  glutPostRedisplay();
}

/* Arrays for current key state */
int keySpecialStates[246]; // Create an array of boolean values of length 246 (0-245)

/* A special key has been pressed */
void keySpecial(int key, int x, int y) {
  keySpecialStates[key] = 1;
}

/* A special key has been released */
void keySpecialUp(int key, int x, int y) {
  keySpecialStates[key] = 0;
}

/**
 * Should be called every display loop
 * Uses the function cursor_keys for every pressed key
 */
void keySpecialOperations(void) {
  int i;
  for (i = 0; i < 246; i++) {
    if (keySpecialStates[i]) {
      specialKeys(i, 0, 0);
    }
  }
}

////////////// Keyboard Presses ///////////////

/**
 * Deal with a "special" key being pressed
 * @param key current key number
 */
void specialKeys(int key, int x, int y) {
  switch (key) {
    case GLUT_KEY_LEFT:
      spin(LEFT);
      break;
    case GLUT_KEY_RIGHT:
      spin(RIGHT);
      break;
    case GLUT_KEY_UP:
      spin(UP);
      break;
    case GLUT_KEY_DOWN:
      spin(DOWN);
      break;
  }
} // cursor_keys()

///////////////////////////////////////////////

/**
 * Position the camera and rotate the model
 */
void positionCamera() {
  gluLookAt(20.0, 10.0 + cameraLoopYPosition, 0.0, //eyeX, eyeY, eyeZ
      0.0, 5.0, 0.0, //centerX, centerY, centerZ
      0.0, 1.0, 0.0); //upX, upY, upZ

  // Rotate the scene
  glRotatef(cameraLoopYAngle, 0.f, 1.f, 0.f); /* orbit the Y axis */

  //glRotatef(cos(DEG_TO_RAD*cameraLoopYAngle) * cameraLoopXAngle, 0.f, 0.f, 1.f); /* orbit the Z axis */
  //glRotatef(sin(DEG_TO_RAD*cameraLoopYAngle) * cameraLoopXAngle, 1.f, 0.f, 0.f); /* orbit the X axis */
}

/**
 * Main draw function
 */
void display() {
  // Clear the screen
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Reset the current matrix
  glLoadIdentity();

  // Set the camera position
  positionCamera();

  // Draw the floor
  glCallList(gridFloorList);

  // If enabled, draw coordinate axis
  if (axisEnabled)
    glCallList(axisList);

  // Draw the emitter
  glPushMatrix();
  glTranslatef(0, 10, 0);
  drawCube(2);
  glPopMatrix();

  // Make the particles larger
  glPointSize(10);

  // Draw every particle
  drawParticles();

  // Draw an fps counter
  drawFPS();

  // Empty buffers
  glFlush();

  // Swap buffer
  glutSwapBuffers();
}

/**
 * Called when OpenGL is idle
 */
void idleTick(void) {
  // Calculate FPS
  calculateFPS();

  // Press any (currently down) keys
  keySpecialOperations();

  // If we've received no user input, then rotate
  if (!rotationKeyboardInputReceived) {
    cameraLoopYAngle += 10 * deltaTime;
  }

  //Calculate position of each each particle
  int i;
  for (i = 0; i < numParticles; i++) {
    //fprintf(stderr, "Delta time: %f\n", deltaTime);
    //fprintf(stderr, "9.8*deltaTime/2: %f\n", 9.8*deltaTime/2);
    //fprintf(stderr, "Old velocity: %f\n", particles[i].velocity.y);

    if (!particles[i].dead) {
      particles[i].velocity.x *= 1 - (deltaTime * 0.01); // drag
      particles[i].position.x = particles[i].position.x
          + particles[i].velocity.x * deltaTime;

      particles[i].velocity.y *= 1 - (deltaTime * 0.01); //drag
      particles[i].velocity.y = particles[i].velocity.y + gravityStrength
          * deltaTime / 2;
      particles[i].position.y = particles[i].position.y
          + particles[i].velocity.y * deltaTime;
      particles[i].velocity.y = particles[i].velocity.y + gravityStrength
          * deltaTime / 2;

      particles[i].velocity.z *= 1 - (deltaTime * 0.01); // drag
      particles[i].position.z = particles[i].position.z
          + particles[i].velocity.z * deltaTime;

      // If we have hit (or are beneath) the floor
      if (!particles[i].yCollision && particles[i].position.y <= 0) {
        particles[i].velocity.y *= -0.6 - randomNumber() * 0.15; // bounce (lose velocity) and go the other way
        particles[i].yCollision = 1;

        // The floor is a bit bumpy, occasionally add other vectors
        if (randomNumber() < 0.25)
          particles[i].velocity.x += randomNumber() - 0.5;

        if (randomNumber() < 0.25)
          particles[i].velocity.z += randomNumber() - 0.5;

      } else if (particles[i].position.y > 0) { // if we're clear
        particles[i].yCollision = 0;
      } else {
        // Delete the particle it's going nowhere
        if (particles[i].position.y <= 0.01 && particles[i].velocity.y <= 0.01) {
          particles[i].dead = 1;
        }
      }

      //fprintf(stderr, "New velocity: %f\n", particles[i].velocity.y);
    }

    // Chance to respawn items that are "dead" after one second
    else {
      if ((particles[i].firstSpawn || particles[i].deadTime > 5)
          && randomNumber() < 0.01) {
        particles[i].position.x = 0;
        particles[i].position.y = 10;
        particles[i].position.z = 0;
        particles[i].velocity.x = randomNumber() * 2 - 1 + randomNumber() - 0.5;
        particles[i].velocity.y = randomNumber() * 2 - 1 + randomNumber() - 0.5;
        particles[i].velocity.z = randomNumber() * 2 - 1 + randomNumber() - 0.5;
        particles[i].xCollision = 0;
        particles[i].yCollision = 0;
        particles[i].zCollision = 0;
        particles[i].r = randomNumber();
        particles[i].g = randomNumber();
        particles[i].b = randomNumber();
        particles[i].dead = 0;
        particles[i].deadTime = 0;
      } else {
        particles[i].deadTime += deltaTime;
      }
    }
  }

  //  Call display function (draw the current frame)
  glutPostRedisplay();
}

///////////////////////////////////////////////

/**
 * Handle window resize
 */
void reshape(int width, int height) {

  WINDOW_WIDTH = glutGet(GLUT_SCREEN_WIDTH);
  WINDOW_HEIGHT = glutGet(GLUT_SCREEN_HEIGHT);

  glViewport(0, 0, (GLsizei) width, (GLsizei) height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(60, (GLfloat) width / (GLfloat) height, 1.0, 10000.0);
  glMatrixMode(GL_MODELVIEW);
}

////////////// Make Display Lists/////////////////

/**
 * Create a display list for drawing coord axis
 */
void makeAxes() {

  axisList = glGenLists(1);
  glNewList(axisList, GL_COMPILE);
  glLineWidth(2.0);
  glBegin(GL_LINES);
  glColor3f(1.0, 0.0, 0.0); // X axis - red
  glVertex3f(0.0, 0.0, 0.0);
  glVertex3f(AXIS_SIZE, 0.0, 0.0);
  glColor3f(0.0, 1.0, 0.0); // Y axis - green
  glVertex3f(0.0, 0.0, 0.0);
  glVertex3f(0.0, AXIS_SIZE, 0.0);
  glColor3f(0.0, 0.0, 1.0); // Z axis - blue
  glVertex3f(0.0, 0.0, 0.0);
  glVertex3f(0.0, 0.0, AXIS_SIZE);
  glEnd();
  glEndList();
}

/**
 * Create a display list for drawing the floor
 * Tweaked from ex1 of COMP27112 by Toby Howard
 */
void makeGridFloor(int size, int yPos) {
  // Move the floor very slightly further down, so objects "on the floor" are slightly above it
  yPos -= 0.002;

  // Create a display list for the floor
  gridFloorList = glGenLists(2);
  glNewList(gridFloorList, GL_COMPILE);

  // Draw a grey floor
  glDepthRange(0.1, 1.0);
  glColor3f(0.4, 0.4, 0.4);
  glBegin(GL_QUADS);
  glVertex3f(-size, 0, size);
  glVertex3f(size, 0, size);
  glVertex3f(size, 0, -size);
  glVertex3f(-size, 0, -size);
  glEnd();

  // Draw grid lines on the floor
  glDepthRange(0.0, 0.9);
  glColor3f(0.2, 0.2, 0.2);
  glLineWidth(1.0);
  glBegin(GL_LINES);
  int x, z;
  for (x = -size; x <= size; x++) {
    glVertex3f((GLfloat) x, yPos + 0.001, -size);
    glVertex3f((GLfloat) x, yPos + 0.001, size);
  }
  for (z = -size; z <= size; z++) {
    glVertex3f(-size, yPos + 0.001, (GLfloat) z);
    glVertex3f(size, yPos + 0.001, (GLfloat) z);
  }
  glEnd();
  glEndList();
}

///////////////////////////////////////////////

/**
 * Rotate the camera
 *  @param direction definedDirection (UP,LEFT,DOWN,RIGHT)
 */
void spin(int direction) {
  rotationKeyboardInputReceived = 1;

  switch (direction) {
    case UP:
      cameraLoopYPosition += 10 * deltaTime;
      break;
    case DOWN:
      cameraLoopYPosition -= 10 * deltaTime;
      break;
    case LEFT:
      cameraLoopYAngle += 60.0f * deltaTime;
      break;
    case RIGHT:
      cameraLoopYAngle -= 60.0f * deltaTime;
      break;
  }

  if (cameraLoopYPosition > 50)
    cameraLoopYPosition = 50;
  if (cameraLoopYPosition < -8)
    cameraLoopYPosition = -8;
  if (cameraLoopYAngle > 360)
    cameraLoopYAngle -= 360;
  if (cameraLoopYAngle < 0)
    cameraLoopYAngle += 360;
}

///////////////////////////////////////////////

void initGraphics(int argc, char *argv[]) {
  glutInit(&argc, argv);

  // Spawn a window
  glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
  glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH) - WINDOW_WIDTH)/2,
                         (glutGet(GLUT_SCREEN_HEIGHT) - WINDOW_HEIGHT)/2);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
  glutCreateWindow("COMP37111 Particles");

  // Connect my function
  glutDisplayFunc(display);
  glutIdleFunc(idleTick);
  glutKeyboardFunc(keyboard);
  glutReshapeFunc(reshape);

  // Keyboard Functions, derived from:
  // http://www.swiftless.com/tutorials/opengl/keyboard.html
  glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);
  glutSpecialFunc(keySpecial); //key press
  glutSpecialUpFunc(keySpecialUp); //key up

  // Sky blue
  glClearColor(0.53f, 0.808f, 0.98f, 0.0f);

  // Enable depth test
  glEnable(GL_DEPTH_TEST);

  // Accept ift closer to the camera
  glDepthFunc(GL_LESS);

  // Smooth points
  glEnable(GL_POINT_SMOOTH);

  // Draw (and save on the GPU) the axes and floor
  makeAxes();
  makeGridFloor(FLOOR_SIZE, 0);
}

/////////////////////////////////////////////////

int main(int argc, char *argv[]) {
  // Seed random
  srand(time(NULL));

  // Boot the graphics engine
  initGraphics(argc, argv);
  glutMainLoop();
}
