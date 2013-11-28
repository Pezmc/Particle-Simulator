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

#ifdef __ECLIPSE__
  #include <glut.h> //GLUT/glut.h
  #include <OpenGL.h> //OpenGL/OpenGL.h
#elif defined(__APPLE__)
  #include <GLUT/glut.h>
  #include <OpenGL/OpenGL.h>
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
#define PARTICLES_PER_EMITTER_LIMIT 1

/* Define Some Keys */
#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3

/* Gravity default */
#define GRAVITY_STRENGTH -9.8

/* Bounce default */
#define BOUNCE_COEFFICIENT 0.6

/* Bounding Box */
#define FLOOR_HEIGHT 0
#define CEILING_HEIGHT 100

/* Items in my menu */
typedef enum
{
        MENU_CUBE_TOP,
        MENU_CUBE_BOTTOM,
        MENU_CUBE_FRONT,
        MENU_CUBE_LEFT,
        MENU_CUBE_BACK,
        MENU_CUBE_RIGHT
} MENU_TYPE;

// Assign a default value
MENU_TYPE CURRENT_MENU_SHOW = MENU_CUBE_TOP;

////////////// Structs //////////////

/*
 * A 3d position (or vector)
 */
typedef struct {
  GLfloat x, y, z;
} Vector;


/*
 * A linked list of positions
 */
struct PositionNode;
typedef struct PositionNode {
  Vector position;
  struct PositionNode *next;

} PositionNode;

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

  // Linked list of positions
  int previousPositionsCount;
  PositionNode *previousPositionsRoot;
  PositionNode *previousPositionsEnd;

} Particle;

/* particle struct */

typedef struct {
  Vector bottomLeft;
  Vector topRight;

  GLfloat r, g, b; // color

  GLfloat yawAngle; // rotation around Y

  Vector spawnVelocity;

  Particle* particles;//[PARTICLES_PER_EMITTER_LIMIT];

  int numberOfParticles;
} SurfaceEmitter;

//////////// Globals //////////////

/* Rotate the scene around */
float cameraLoopYPosition = 0;
float cameraLookYPosition = 0;
float cameraLoopYAngle = 45;

/* Emitter Y height */
float emitterYPosition = 0;

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
int WORLD_SIZE = 100;

/* Floor ID */
GLuint gridFloorListID;

/* Ceiling position */
GLuint ceilingListID;

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

/* How bouncy is the floor? */
float bounceCoefficient = BOUNCE_COEFFICIENT;

/* Our array of emitters */
SurfaceEmitter *emitters;
int emitterCount = 0;

///////////////// Helpers ////////////////////

/**
 * Return a random double between 0 and 1
 */
double randomBetween(double min, double max) {
  double range = max - min;
  return (rand() / (double) RAND_MAX) * range + min;
}

double randomMax(double max) {
  return randomBetween(0,max);
}

double randomNumber() {
  return randomMax(1);
}

/////////////// Main Draw ///////////////////

/* Draw an individual particle */
void drawParticle(Particle particle) {
  glBegin(GL_POINTS);
    glColor3f(particle.r, particle.g, particle.b); // color
    glVertex3f(particle.position.x, particle.position.y, particle.position.z); // position
  glEnd();
}

/* Draw all the particles on the screen */
void drawParticles(SurfaceEmitter emitter) {
  // Draw each particle for the emitter
  int i;
  for (i = 0; i < emitter.numberOfParticles; i++) {
    drawParticle(emitter.particles[i]);
  }
}

/* Draw all the emitters on the screen */
void drawEmitters() {
  int i;
  for(i = 0; i < emitterCount; i++) {

    glPushMatrix();
      // Rotation
      //glRotatef(emitters[i].yawAngle, 0.f, 1.f, 0.f); /* rotate on the Y axis */

      // Draw the emitter plane
      glBegin(GL_POLYGON);
        glColor3f(emitters[i].r, emitters[i].g, emitters[i].b);
  
        // Top right corner
        glVertex3f(emitters[i].topRight.x,   emitters[i].topRight.y,   emitters[i].topRight.z);
  
        // If Y is equal we are drawing the bottomRight (from above)
        if(emitters[i].bottomLeft.y == emitters[i].topRight.y)
          glVertex3f(emitters[i].topRight.x, emitters[i].topRight.y,   emitters[i].bottomLeft.z);
        else
          glVertex3f(emitters[i].topRight.x, emitters[i].bottomLeft.y, emitters[i].topRight.z);
  
        // Bottom left corner
        glVertex3f(emitters[i].bottomLeft.x, emitters[i].bottomLeft.y, emitters[i].bottomLeft.z);
  
        // If Y is equal we are drawing the topLeft (from above)
        if(emitters[i].bottomLeft.y == emitters[i].topRight.y)
          glVertex3f(emitters[i].bottomLeft.x, emitters[i].topRight.y,   emitters[i].topRight.z);
        else
          glVertex3f(emitters[i].bottomLeft.x, emitters[i].topRight.y,   emitters[i].bottomLeft.z);
      glEnd();

      drawParticles(emitters[i]);
    glPopMatrix();
  }
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
  if (lightingOn) glDisable(GL_LIGHTING);

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
  if (lightingOn)  glEnable(GL_LIGHTING);
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
  drawString(GLUT_BITMAP_HELVETICA_10, 0.8, 0.97, str);
}

/**
 * Draw the current gravity on the screen
 */
void drawGravity() {
  char str[80];
  sprintf(str, "Gravity Strength: %2.1f", gravityStrength * -1);

  //  Print the FPS to the window
  drawString(GLUT_BITMAP_HELVETICA_10, 0.8, 0.95, str);
}

/**
 * Draw the current bounce on the screen
 */
void drawBounce() {
  char str[80];
  sprintf(str, "Bounce Strength: %2.2f", bounceCoefficient);

  //  Print the FPS to the window
  drawString(GLUT_BITMAP_HELVETICA_10, 0.8, 0.93, str);
}

///////////////////////////////////////////////

/**
 * Position the camera and rotate the model
 */
void positionCamera() {
  gluLookAt(20.0, 10.0 + cameraLoopYPosition, 0.0, //eyeX, eyeY, eyeZ
      0.0, 5.0 + cameraLookYPosition, 0.0, //centerX, centerY, centerZ
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

  // Draw the ceiling
  glCallList(ceilingListID);

  // Draw the floor
  glCallList(gridFloorListID);

  // If enabled, draw coordinate axis
  if (axisEnabled)
    glCallList(axisList);

  // Gravity prompt
  if (gravityStrength + 0.1 < GRAVITY_STRENGTH || gravityStrength - 0.1 > GRAVITY_STRENGTH) {
    drawGravity();
  }

  // Bounce
  if (bounceCoefficient + 0.01 < BOUNCE_COEFFICIENT || bounceCoefficient - 0.01 > BOUNCE_COEFFICIENT) {
    drawBounce();
  }

  // Make the particles larger
  glPointSize(10);

  // Draw every emitter
  drawEmitters();

  // Draw an fps counter
  drawFPS();

  // Empty buffers
  glFlush();

  // Swap buffer
  glutSwapBuffers();
}

///////////////////////////////////////////////

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

/**
 * Calculate each particles position
 */
void calculateParticle(Particle *particle, SurfaceEmitter *emitter) {

  // If the particle has been killed
  if(particle->dead) {

    // Free all the memory being used for previous particles
    /*struct PositionNode* currentNode = particle->previousPositionsRoot;
    struct PositionNode* oldNode;
    if( currentNode && currentNode != NULL ) {
      while ( currentNode->next ) {
        oldNode = currentNode;
        currentNode = currentNode->next;

        free(oldNode);
      }
    }*/
    //particle->previousPositionsRoot = 0; // point to nothing
    //particle->previousPositionsCount = 0;

    // Respawn them here
    if((particle->firstSpawn || particle->deadTime > 5) && randomNumber() < 0.001) {

      // Spawn in a random xyz between top left and bottom right
      particle->position.x = emitter->bottomLeft.x + (emitter->topRight.x - emitter->bottomLeft.x) * randomBetween(0.25,0.75);
      particle->position.y = emitter->bottomLeft.y + (emitter->topRight.y - emitter->bottomLeft.y) * randomBetween(0.25,0.75);
      particle->position.z = emitter->bottomLeft.z + (emitter->topRight.z - emitter->bottomLeft.z) * randomBetween(0.25,0.75);

      // Spawn the particle with the emitters velocity
      particle->velocity.x = emitter->spawnVelocity.x + randomBetween(-1,1);
      particle->velocity.y = emitter->spawnVelocity.y + randomBetween(-1,1);
      particle->velocity.z = emitter->spawnVelocity.z + randomBetween(-1,1);

      // Colliding
      particle->yCollision = 0;

      // Particle is "alive"
      particle->dead = 0;
      particle->deadTime = 0;

      // Malloc the linked list item #1
      PositionNode* newPositionsRoot = malloc(sizeof(struct PositionNode));
      if (newPositionsRoot == 0) {
        fprintf(stderr, "Out of memory" );
        return;
      }
      newPositionsRoot->position.x = particle->position.x;
      newPositionsRoot->position.y = particle->position.y;
      newPositionsRoot->position.z = particle->position.z;
      particle->previousPositionsRoot = newPositionsRoot;

      // Color
      if(emitter->r > 0) particle->r = emitter->r + randomBetween(-0.1,0.1);
      else particle->r = randomMax(0.75);

      if(emitter->g > 0) particle->g = emitter->g + randomBetween(-0.1,0.1);
      else particle->g = randomMax(0.75);

      if(emitter->b > 0) particle->b = emitter->b + randomBetween(-0.1,0.1);
      else particle->b = randomMax(0.75);
    } else {
      particle->deadTime += deltaTime;
    }

  }
  // Particle is alive
  else {

    // Store the particles previous position
    if(particle->previousPositionsCount < 100) {
      PositionNode* newPositionsRoot = malloc(sizeof(struct PositionNode));
      //PositionNode* oldRoot = particle->previousPositionsRoot;
      if (newPositionsRoot == 0) {
        fprintf(stderr, "Out of memory" );
        exit(0);
        return;
      }
      newPositionsRoot->position.x = particle->position.x;
      newPositionsRoot->position.y = particle->position.y;
      newPositionsRoot->position.z = particle->position.z;
      newPositionsRoot->next = particle->previousPositionsRoot; // old root
      particle->previousPositionsRoot = newPositionsRoot;
      particle->previousPositionsCount++;
    }

    // Movement in X
    particle->velocity.x *= 1 - (deltaTime * 0.01); // drag
    particle->position.x = particle->position.x + particle->velocity.x * deltaTime;

    // Movement in Y (+ gravity)
    particle->velocity.y *= 1 - (deltaTime * 0.01); // drag
    particle->velocity.y = particle->velocity.y + gravityStrength * deltaTime / 2; // gravity #1
    particle->position.y = particle->position.y + particle->velocity.y * deltaTime; // update position
    particle->velocity.y = particle->velocity.y + gravityStrength * deltaTime / 2; // gravity #2

    // Movement in Z
    particle->velocity.z *= 1 - (deltaTime * 0.01); // drag
    particle->position.z = particle->position.z + particle->velocity.z * deltaTime;

    // If we are below the floor, or above the "ceiling"
    if (!particle->yCollision && (particle->position.y <= FLOOR_HEIGHT || particle->position.y >= CEILING_HEIGHT)) { // floor
      particle->velocity.y *= -bounceCoefficient - randomBetween(0, 0.15); // bounce (lose velocity) and go the other way
      particle->yCollision = 1;

      // The floor is a bit bumpy, occasionally add other forces
      if (randomNumber() < 0.25)
        particle->velocity.x += randomBetween(-0.5,0.5);
      if (randomNumber() < 0.25)
        particle->velocity.z += randomBetween(-0.5,0.5);

    }
    // If we're clear of the floor
    else if (particle->position.y > FLOOR_HEIGHT && particle->position.y < CEILING_HEIGHT) {
      particle->yCollision = 0;

    }
    // Particle below floor (or above ceiling) and currently colliding
    else {

      // Delete the particle it's below the floor
      if (particle->position.y <= FLOOR_HEIGHT + 0.01 && particle->velocity.y <= 0.01) {
        particle->dead = 1;
      }
      // Delete the particle if it's above the ceiling
      else if(particle->position.y >= CEILING_HEIGHT - 0.01 && particle->velocity.y >= -0.01) {
        particle->dead = 1;
      }

    }

  } // particle alive
}

/**
 * Calculate updates on an emitters particles
 */
void calculateParticles(SurfaceEmitter *emitter) {
  int i;
  for (i = 0; i < emitter->numberOfParticles; i++) {
    calculateParticle(&emitter->particles[i], emitter);
  }
}

/**
 * Calculate updates on each emitter
 */
void calculateEmitters() {
  int i;
  for (i = 0; i < emitterCount; i++) {
      calculateParticles(&emitters[i]);

      // Rotate very slowly
      //emitters[i].yawAngle += 30 * deltaTime;

      // Move emitters up 
      emitters[i].topRight.y += emitterYPosition;
      emitters[i].bottomLeft.y += emitterYPosition;
     
  }
      
  // Reset the extra Y
  emitterYPosition = 0;
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
      cameraLoopYPosition += 15 * deltaTime;
      break;
    case DOWN:
      cameraLoopYPosition -= 15 * deltaTime;
      break;
    case LEFT:
      cameraLoopYAngle += 60.0f * deltaTime;
      break;
    case RIGHT:
      cameraLoopYAngle -= 60.0f * deltaTime;
      break;
  }

  if (cameraLoopYPosition > CEILING_HEIGHT - 12) cameraLoopYPosition = CEILING_HEIGHT - 12;
  if (cameraLoopYPosition < -9) cameraLoopYPosition = -9;
  if (cameraLoopYAngle > 360) cameraLoopYAngle -= 360;
  if (cameraLoopYAngle < 0) cameraLoopYAngle += 360;
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
    case GLUT_KEY_F1:
      gravityStrength += 5 * deltaTime;
      break;
    case GLUT_KEY_F2:
      gravityStrength -= 5 * deltaTime;
      break;
    case GLUT_KEY_F3:
      bounceCoefficient -= 0.5 * deltaTime;
      break;
    case GLUT_KEY_F4:
      bounceCoefficient += 0.5 * deltaTime;
      break;
    case GLUT_KEY_F5:
      emitterYPosition -= 10 * deltaTime;
      break;
    case GLUT_KEY_F6:
      emitterYPosition += 10 * deltaTime;
      break;
    case GLUT_KEY_F11:
      cameraLookYPosition -= 20 * deltaTime;
      break;
    case GLUT_KEY_F12:
      cameraLookYPosition += 20 * deltaTime;
      break;
  }
} // cursor_keys()

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
void keySpecialOperations() {
  int i;
  for (i = 0; i < 246; i++) {
    if (keySpecialStates[i]) {
      specialKeys(i, 0, 0);
    }
  }
}

///////////////////////////////////////////////

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

  // Calculate everything!
  calculateEmitters();

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
/*void makeGridFloor(int size, int yPos) {
  // Move the floor very slightly further down, so objects "on the floor" are slightly above it
  yPos -= 0.001;

  // Create a display list for the floor
  gridFloorListID = glGenLists(2);
  glNewList(gridFloorListID, GL_COMPILE);

    // Draw a grey floor
    glDepthRange(0.1, 1.0);
    glColor3f(0.4, 0.4, 0.4);
    glBegin(GL_QUADS);
      glVertex3f(-size, yPos, size);
      glVertex3f(size, yPos, size);
      glVertex3f(size, yPos, -size);
      glVertex3f(-size, yPos, -size);
    glEnd();

    // Draw grid lines on the floor
    glDepthRange(0.0, 0.9);
    glColor3f(0.2, 0.2, 0.2);
    glLineWidth(1.0);
    glBegin(GL_LINES);
    int x, z;
    for (x = -size; x <= size; x++) {
      glVertex3f((GLfloat) x, yPos + 2, -size);
      glVertex3f((GLfloat) x, yPos + 2, size);
    }
    for (z = -size; z <= size; z++) {
      glVertex3f(-size, yPos + 2, (GLfloat) z);
      glVertex3f(size, yPos + 2, (GLfloat) z);
    }
    glEnd();

  glEndList();
}*/
/**
 * Create a display list for drawing the floor
 * Tweaked from ex1 of COMP27112 by Toby Howard
 */
void makeGridFloor(int size, int yPos) {
  // Move the floor very slightly further down, so objects "on the floor" are slightly above it
  yPos -= 0.002;

  // Create a display list for the floor
  gridFloorListID = glGenLists(2);
  glNewList(gridFloorListID, GL_COMPILE);

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

void makeCeiling(int size, int yPos) {
  // Create a display list for the ceiling
  ceilingListID = glGenLists(3);
  glNewList(ceilingListID, GL_COMPILE);

    // Draw a grey ceiling
    glDepthRange(0.1, 1.0);
    glColor3f(0.3, 0.3, 0.3);
    glBegin(GL_QUADS);
      glVertex3f(-size, yPos, size);
      glVertex3f(size, yPos, size);
      glVertex3f(size, yPos, -size);
      glVertex3f(-size, yPos, -size);
    glEnd();

  glEndList();
}



///////////////////////////////////////////////

// Menu handling function definition
void selectMenuItem(int item)
{
        switch (item)
        {
	case MENU_CUBE_TOP:
	case MENU_CUBE_BOTTOM:
	case MENU_CUBE_FRONT:
	case MENU_CUBE_LEFT:
	case MENU_CUBE_BACK:
	case MENU_CUBE_RIGHT:
                CURRENT_MENU_SHOW = (MENU_TYPE) item;
                break;
        default:
                break;
        }

        glutPostRedisplay();

        return;
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


  int cubeMenu = glutCreateMenu( selectMenuItem );
  glutAddMenuEntry("Set Color", MENU_CUBE_TOP );
  glutAddMenuEntry("Set Particles", MENU_CUBE_TOP );
  glutAddMenuEntry("Set Velocity", MENU_CUBE_TOP );

  // Create a menu
  glutCreateMenu( selectMenuItem );

  // Add menu items
  glutAddSubMenu("Top Face", cubeMenu);
  //glutAddMenuEntry("Top Face", MENU_CUBE_TOP);
  glutAddMenuEntry("Bottom Face", MENU_CUBE_BOTTOM);
  glutAddMenuEntry("Front Face", MENU_CUBE_FRONT);
  glutAddMenuEntry("Left Face", MENU_CUBE_LEFT);
  glutAddMenuEntry("Back Face", MENU_CUBE_BACK);
  glutAddMenuEntry("Right Face", MENU_CUBE_RIGHT);

  // Associate a mouse button with menu
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  // Draw (and save on the GPU) the axes and floor
  makeAxes();
  makeGridFloor(WORLD_SIZE, FLOOR_HEIGHT);
  makeCeiling(WORLD_SIZE, CEILING_HEIGHT);
}

/////////////////////////////////////////////////

void createEmitter(int id) {
  // Defaults
  emitters[id].r = 0;
  emitters[id].g = 0;
  emitters[id].b = 0;
  emitters[id].bottomLeft.x = 0;
  emitters[id].bottomLeft.y = 10;
  emitters[id].bottomLeft.z = 0;
  emitters[id].topRight.x = 0;
  emitters[id].topRight.y = 10;
  emitters[id].topRight.z = 0;
  emitters[id].spawnVelocity.x = 0;
  emitters[id].spawnVelocity.y = 0;
  emitters[id].spawnVelocity.z = 0;
  emitters[id].yawAngle = 0;

  int i;
  emitters[id].particles = (Particle*)calloc(PARTICLES_PER_EMITTER_LIMIT, sizeof(Particle));
  for(i = 0; i < PARTICLES_PER_EMITTER_LIMIT; i++) {
    //emitters[id].particles[i] = (Particle*)calloc(1, sizeof(Particle));
    emitters[id].particles[i].dead = 1;
    emitters[id].particles[i].deadTime = 0;
    emitters[id].particles[i].firstSpawn = 1;
    emitters[id].particles[i].previousPositionsCount = 0;
    emitters[id].particles[i].previousPositionsRoot = NULL;
    emitters[id].particles[i].previousPositionsEnd = NULL;
  }
  emitters[id].numberOfParticles = PARTICLES_PER_EMITTER_LIMIT;

  // Details for this ID
  switch(id) {
    case 0: // bottom
      emitters[id].r = 1;
      emitters[id].bottomLeft.x = -1;
      emitters[id].bottomLeft.z = -1;
      emitters[id].topRight.x = 1;
      emitters[id].topRight.z = 1;
      emitters[id].spawnVelocity.y = -5;
    break;
    case 1: // front
      emitters[id].g = 1;
      emitters[id].bottomLeft.x = -1;
      emitters[id].bottomLeft.z = -1;
      emitters[id].topRight.x = 1;
      emitters[id].topRight.y = 12;
      emitters[id].topRight.z = -1;
      emitters[id].spawnVelocity.z = -5;
    break;
    case 2: //left
      emitters[id].b = 1;
      emitters[id].bottomLeft.x = -1;
      emitters[id].bottomLeft.y = 12;
      emitters[id].bottomLeft.z = 1;
      emitters[id].topRight.x = -1;
      emitters[id].topRight.z = -1;
      emitters[id].spawnVelocity.x = -5;
    break;
    case 3: // back
      emitters[id].g = 1;
      emitters[id].b = 1;
      emitters[id].bottomLeft.x = 1;
      emitters[id].bottomLeft.z = 1;
      emitters[id].topRight.x = -1;
      emitters[id].topRight.y = 12;
      emitters[id].topRight.z = 1;
      emitters[id].spawnVelocity.z = 5;
    break;
    case 4: // right
      emitters[id].r = 0.75;
      emitters[id].b = 0.75;
      emitters[id].bottomLeft.x = 1;
      emitters[id].bottomLeft.z = -1;
      emitters[id].topRight.x = 1;
      emitters[id].topRight.y = 12;
      emitters[id].topRight.z = 1;
      emitters[id].spawnVelocity.x = 5;
    break;
    case 5: // top
      emitters[id].r = 1;
      emitters[id].g = 1;
      emitters[id].bottomLeft.x = -1;
      emitters[id].bottomLeft.y = 12;
      emitters[id].bottomLeft.z = -1;
      emitters[id].topRight.x = 1;
      emitters[id].topRight.y = 12;
      emitters[id].topRight.z = 1;
      emitters[id].numberOfParticles = 100;
      emitters[id].spawnVelocity.y = 10;
    break;
  }

}

void createEmitters() {

  int e;

  emitters = calloc(EMITTER_LIMIT, sizeof(SurfaceEmitter));

  emitterCount = EMITTER_LIMIT;
  for(e = 0; e < emitterCount; e++) {
    createEmitter(e);
  }
}

/////////////////////////////////////////////////

int main(int argc, char *argv[]) {
  // Seed random
  srand(time(NULL));

  createEmitters();
  return 0;

  // Boot the graphics engine
  initGraphics(argc, argv);
  glutMainLoop();

  return 0;
}
