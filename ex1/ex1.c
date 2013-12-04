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
  #include <GLUT/glut.h> //GLUT/glut.h
  #include <OpenGL/OpenGL.h> //OpenGL/OpenGL.h
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
#define PARTICLES_PER_EMITTER_LIMIT 165000
#define DEFAULT_PARTICLE_LIMIT 6000

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

/* What gravity type are we using? */
typedef enum
{
    VERLET_APPROXIMATION,
    REAL_GRAVITY
} GRAVITY_TYPE;

/* Items in my menu */
typedef enum
{
    MENU_GLOBAL_SETTINGS,
    MENU_LOCAL_SETTINGS
} MENU_MAIN_TYPE;

typedef enum
{
    MENU_GLOBAL_BOUNCE,
    MENU_GLOBAL_GRAVITY,
    MENU_GLOBAL_PARTICLES,
    MENU_GLOBAL_LIFETIME
} MENU_GLOBAL_TYPE;

typedef enum
{
    MENU_LOCAL_COLOR,
    MENU_LOCAL_PARTICLES,
    MENU_LOCAL_VELOCITY
} MENU_LOCAL_TYPE;

// Assign a default value
MENU_MAIN_TYPE CURRENT_MENU_SHOW = MENU_GLOBAL_SETTINGS;

// Colors
typedef enum
{
    RED, PINK, ORANGE, YELLOW, GREEN, BLUE, INDIGO, VIOLET, WHITE, BLACK
} COLOR;

typedef enum
{
    POINT,
    JITTER_POINTS,
    LINES,
    BILLBOARD
} RENDER_TYPE;

typedef enum
{
  WATERFALL,
  FOUNTAIN,
  ROCKET,
  FIRE_EMITTER,
  DEFAULT
} PRESET;

typedef enum
{
	TOGGLE_FLOOR
} MAIN_MENU;

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
  GLfloat alpha; // how transparent

  Vector position; // current position
  Vector previousPosition; // last frame position
  Vector velocity; //current velocity

  // Currently colliding with the floor?
  int yCollision;

  // "dead" = Not moving
  int dead;
  float deadTime;

  // how alive
  float lifeTime;

  // Should we draw it?
  int display;

  // Spawned yet?
  int firstSpawn;

  // Linked list of positions
  //PositionNode *previousPositions;

} Particle;

/* particle struct */

typedef struct {
  Vector bottomLeft;
  Vector topRight;

  GLfloat r, g, b; // color

  GLfloat yawAngle; // rotation around Y

  Vector spawnVelocity;

  float velocityMultiplier;

  Particle particles[PARTICLES_PER_EMITTER_LIMIT];

  int particleLifeTime;

  int numberOfParticles;
} SurfaceEmitter;

//////////// Globals //////////////

/* Rotate the scene around */
float cameraLoopYPosition = 0;
float cameraLookYPosition = 0;
float cameraLoopYAngle = 45;
float cameraDistance = 0;

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
int axisEnabled = 0;

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
float deltaTime = 0.001;
float deltaTimeSpeed = deltaTime;

/* How strong is gravity at the moment */
float gravityStrength = GRAVITY_STRENGTH;
int gravity = 1;

/* How bouncy is the floor? */
float bounceCoefficient = BOUNCE_COEFFICIENT;

/* Our array of emitters */
SurfaceEmitter emitters[EMITTER_LIMIT];
int emitterCount = 0;

/* Global settings */
int currentEmitterParticleLimit = 1000;
int currentParticleLimit = DEFAULT_PARTICLE_LIMIT;
int paused = 0;
float speed = 1;
int selectedCubeFace = -1;
int drawFloor = 1;

RENDER_TYPE currentRenderType = POINT;

/* What gravity type are we using? */
GRAVITY_TYPE currentGravityType = REAL_GRAVITY;

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
  return (rand() / (double) RAND_MAX);
  //return randomMax(1); // replaced for speed
}

/////////////// Main Draw ///////////////////

/* Draw an individual particle */
void drawPointParticle(Particle particle) {
  glBegin(GL_POINTS);
    glColor3f(particle.r, particle.g, particle.b); // color
    glVertex3f(particle.position.x, particle.position.y, particle.position.z); // position
  glEnd();
}

/* Draw random particles near this particle */
void drawJitterPointParticle(Particle particle) {
  glBegin(GL_POINTS);
    int i;
    for (i = 0; i < 10; i++) {
      glColor3f(particle.r + randomBetween(-0.1,0.1), particle.g + randomBetween(-0.1,0.1), particle.b + randomBetween(-0.1,0.1)); // color
      glVertex3f(particle.position.x + randomBetween(-0.5,0.5), particle.position.y + randomBetween(-0.5,0.5), particle.position.z + randomBetween(-0.5,0.5));
    }
  glEnd();
}

/* Draw each particle as a 2d square */
void drawBillboard(Particle particle) {

  double size = 0.5 / 2;

  glBegin(GL_POLYGON);

    glColor4f(particle.r, particle.g, particle.b, particle.alpha); // color

    glVertex3f(particle.position.x-size, particle.position.y-size, particle.position.z);
    glVertex3f(particle.position.x-size, particle.position.y+size, particle.position.z);
    glVertex3f(particle.position.x+size, particle.position.y+size, particle.position.z);
    glVertex3f(particle.position.x+size, particle.position.y-size, particle.position.z);

  glEnd();

}

void drawLine(Particle particle) {

  glLineWidth(3);

  glColor3f(particle.r, particle.g, particle.b); // color

  glBegin(GL_LINES);
    glVertex3f(particle.previousPosition.x, particle.previousPosition.y, particle.previousPosition.z);
    glVertex3f(particle.position.x, particle.position.y, particle.position.z);
  glEnd();

}

/* Draw all the particles on the screen */
void drawParticles(SurfaceEmitter emitter) {

  // Draw each particle for the emitter
  int i;

  if (currentRenderType == POINT) {
    for (i = 0; i < emitter.numberOfParticles; i++) {
      if(emitter.particles[i].display)
        drawPointParticle(emitter.particles[i]);
    }
  }

  else if (currentRenderType == JITTER_POINTS) {
    for (i = 0; i < emitter.numberOfParticles; i++) {
      if(emitter.particles[i].display)
        drawJitterPointParticle(emitter.particles[i]);
    }
  }

  else if (currentRenderType == BILLBOARD) {

    for (i = 0; i < emitter.numberOfParticles; i++) {
      if(emitter.particles[i].display)
        drawBillboard(emitter.particles[i]);
    }

  } else if(currentRenderType == LINES) {
    for (i = 0; i < emitter.numberOfParticles; i++) {
      if(emitter.particles[i].display)
        drawLine(emitter.particles[i]);
    }
  }


}

/* Draw all the emitters on the screen */
int blink = 0;
void drawEmitters() {
  int i;
  for(i = 0; i < emitterCount; i++) {

    glPushMatrix();
      // Rotation
      //glRotatef(emitters[i].yawAngle, 0.f, 1.f, 0.f); /* rotate on the Y axis */

      // Draw the emitter plane
      glBegin(GL_POLYGON);

        if(i == selectedCubeFace) {

          if(blink > 50) {
            glColor3f(0.7, 0.7, 0.7);
          } else {
            glColor3f(emitters[i].r, emitters[i].g, emitters[i].b);
          }

          if(blink > 100)
            blink = 0;
          blink++;

        } else
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

/**
 * Draw the current particle count on the screen
 */
void drawParticleCount() {
  char str[80];
  sprintf(str, "Particle Count: %d", currentParticleLimit);

  //  Print the FPS to the window
  drawString(GLUT_BITMAP_HELVETICA_10, 0.8, 0.91, str);
}

void drawSpeed() {
  char str[80];
  sprintf(str, "Speed: %3.0f%%", speed * 100);

  //  Print the FPS to the window
  drawString(GLUT_BITMAP_HELVETICA_10, 0.8, 0.89, str);
}

///////////////////////////////////////////////

/**
 * Position the camera and rotate the model
 */
void positionCamera() {
  gluLookAt(20.0, 10.0 + cameraLoopYPosition, 0.0, //eyeX, eyeY, eyeZ
      0.0, 5.0 + cameraLookYPosition, 0.0, //centerX, centerY, centerZ
      0.0, 1.0, 0.0); //upX, upY, upZ

  // Move the camera "backwards"
  glTranslatef(-cameraDistance, 0.0, 0.0);

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

  if(drawFloor) {
	  // Draw the ceiling
	  glCallList(ceilingListID);

	  // Draw the floor
	  glCallList(gridFloorListID);
  }

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

  // Particles
  if (currentParticleLimit != DEFAULT_PARTICLE_LIMIT) {
    drawParticleCount();
  }

  // Speed
  if (speed != 1) {
    drawSpeed();
  }

  // Make the particles larger
  glPointSize(5); //10

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

  // Delta time is used for physics
  deltaTime = timeSinceLastFrameMS / (1000.0f);

  // Delete time speed is used for physics that takes into account the current speed
  deltaTimeSpeed = deltaTime * speed;

  lastFrameTime = currentTime;
}

void respawnParticle(Particle *particle, SurfaceEmitter *emitter) {
  // Spawn in a random xyz between top left and bottom right of the emitter
  particle->position.x = emitter->bottomLeft.x + (emitter->topRight.x - emitter->bottomLeft.x) * randomBetween(0.25,0.75);
  particle->position.y = emitter->bottomLeft.y + (emitter->topRight.y - emitter->bottomLeft.y) * randomBetween(0.25,0.75);
  particle->position.z = emitter->bottomLeft.z + (emitter->topRight.z - emitter->bottomLeft.z) * randomBetween(0.25,0.75);

  // Spawn the particle with the emitters velocity
  particle->velocity.x = (emitter->spawnVelocity.x * emitter->velocityMultiplier) + randomBetween(-1,1);
  particle->velocity.y = (emitter->spawnVelocity.y * emitter->velocityMultiplier) + randomBetween(-1,1);
  particle->velocity.z = (emitter->spawnVelocity.z * emitter->velocityMultiplier) + randomBetween(-1,1);

  // If we're using verlet we need to set the previous position
  if(currentGravityType == VERLET_APPROXIMATION) {

	// Add default velocity
	particle->previousPosition.x = particle->position.x - (particle->velocity.x * 0.05);
	particle->previousPosition.y = particle->position.y - (particle->velocity.y * 0.05);
	particle->previousPosition.z = particle->position.z - (particle->velocity.z * 0.05);

  }

  // Colliding
  particle->yCollision = 0;

  // Particle is "alive"
  particle->dead = 0;
  particle->deadTime = 0;
  particle->display = 1;
  particle->lifeTime = 0;

  // Color
  if(emitter->r > 0) particle->r = emitter->r + randomBetween(-0.1,0.1);
  else particle->r = randomMax(0.75);

  if(emitter->g > 0) particle->g = emitter->g + randomBetween(-0.1,0.1);
  else particle->g = randomMax(0.75);

  if(emitter->b > 0) particle->b = emitter->b + randomBetween(-0.1,0.1);
  else particle->b = randomMax(0.75);

  // Alpha
  particle->alpha = 0.75 + randomBetween(-0.5,0.5);
  if(particle->alpha > 1)
	particle->alpha = 1;
}

void handleCollisions(Particle *particle) {

	// If we are below the floor, or above the "ceiling"
	if (!particle->yCollision && (particle->position.y <= FLOOR_HEIGHT || particle->position.y >= CEILING_HEIGHT)) { // floor

	  if(currentGravityType == REAL_GRAVITY) {
		particle->velocity.y *= -bounceCoefficient - randomBetween(0, 0.15); // bounce (lose velocity) and go the other way

		// The floor is a bit bumpy, occasionally add other forces
		if (randomNumber() < 0.25)
		  particle->velocity.x += randomBetween(-0.5,0.5);
		if (randomNumber() < 0.25)
		  particle->velocity.z += randomBetween(-0.5,0.5);

	  } else if(currentGravityType == VERLET_APPROXIMATION) {

		// Swap the particles position in y (go back the other way)
		Vector temp = particle->previousPosition;
		particle->previousPosition.y = particle->position.y;

		// Dampen the velocity slightly when swapping
		particle->position.y += (temp.y - particle->position.y) * bounceCoefficient;

	  }
	  particle->yCollision = 1;

	}
	// If we're clear of the floor
	else if (particle->position.y > FLOOR_HEIGHT && particle->position.y < CEILING_HEIGHT) {
	  particle->yCollision = 0;

	}
	// Particle below floor (or above ceiling) and currently colliding
	else {

	  if(currentGravityType == REAL_GRAVITY) {
		// Delete the particle it's below the floor
		if (particle->position.y <= FLOOR_HEIGHT + 0.01 && particle->velocity.y <= 0.01) {
		  particle->dead = 1;
		}
		// Delete the particle if it's above the ceiling
		else if(particle->position.y >= CEILING_HEIGHT - 0.01 && particle->velocity.y >= -0.01) {
		  particle->dead = 1;
		}
	  } else if(currentGravityType == VERLET_APPROXIMATION) {
		// Delete the particle it's below the floor and not moving
		if (particle->position.y <= FLOOR_HEIGHT + 0.01 && particle->position.y - particle->previousPosition.y  <= 0.05) {
		  particle->dead = 1;
		}
		// Delete the particle if it's above the ceiling
		else if(particle->position.y >= CEILING_HEIGHT - 0.01 && particle->position.y - particle->previousPosition.y >= -0.05) {
		  particle->dead = 1;
		}
	  }

	}
}

/**
 * Handle the movement of a particle according to gravity
 */
void handleMovement(Particle *particle, float deltaTimeSpeed) {

	// Increase the life of the particle
	particle->lifeTime += deltaTimeSpeed;

	// Drag
	particle->velocity.x *= 1 - (deltaTimeSpeed * 0.01); // drag
	particle->velocity.y *= 1 - (deltaTimeSpeed * 0.01); // drag
	particle->velocity.z *= 1 - (deltaTimeSpeed * 0.01); // drag

	if(currentGravityType == REAL_GRAVITY) {

	  // Movement in X
	  particle->position.x = particle->position.x + particle->velocity.x * deltaTimeSpeed;

	  if(gravity) {
		  // Movement in Y (+ gravity)
		  particle->velocity.y = particle->velocity.y + gravityStrength * deltaTimeSpeed / 2; // gravity #1
		  particle->position.y = particle->position.y + particle->velocity.y * deltaTimeSpeed; // update position
		  particle->velocity.y = particle->velocity.y + gravityStrength * deltaTimeSpeed / 2; // gravity #2
	  } else {
		  particle->position.y = particle->position.y + particle->velocity.y * deltaTimeSpeed;
	  }

	  // Movement in Z
	  particle->position.z = particle->position.z + particle->velocity.z * deltaTimeSpeed;

	}
	else if(currentGravityType == VERLET_APPROXIMATION) {

	  Vector temp = particle->position;

	  //P.Position += P.Position - P.OldPosition + P.Acceleration*Timestep*Timestep;
	  particle->position.x += particle->position.x - particle->previousPosition.x;
	  if(gravity) {
		  particle->position.y += particle->position.y - particle->previousPosition.y + gravityStrength * 0.001; // Fixed gravity effect
	  } else {
		  particle->position.y += particle->position.y - particle->previousPosition.y;
	  }
	  particle->position.z += particle->position.z - particle->previousPosition.z;

	  // Store previous position
	  particle->previousPosition.x = temp.x;
	  particle->previousPosition.y = temp.y;
	  particle->previousPosition.z = temp.z;

	}
}

/**
 * Calculate each particles position
 */
void calculateParticle(Particle *particle, SurfaceEmitter *emitter) {

  // Data time, taking into account requested speed
  // float deltaTimeSpeed = deltaTime * speed;

  // If the particle has been killed
  if(particle->dead) {

    // Respawn them here
    if(randomNumber() < 0.001) {
      respawnParticle(particle, emitter);
    } else {
      particle->deadTime += deltaTimeSpeed;
      particle->lifeTime += deltaTimeSpeed;
    }

  }
  // Particle is alive
  else {

	// Gravity and velocity
	handleMovement(particle, deltaTimeSpeed);

    // If its life is over force a respawn skipping "dead"
    if(particle->lifeTime > emitter->particleLifeTime && randomNumber() > 0.9) {
    	respawnParticle(particle, emitter);
    }

    // If the floor and ceiling exist
    if(drawFloor) {
    	handleCollisions(particle);
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
 * Set the color of an emitter to a known preset
 */
void setEmitterColor(int faceID, COLOR color) {
  emitters[faceID].r = 0;
  emitters[faceID].g = 0;
  emitters[faceID].b = 0;

  switch(color) {
    case RED:
      emitters[faceID].r = 1;
    break;

    case PINK:
      emitters[faceID].r = 1;
      emitters[faceID].g = 0.4;
      emitters[faceID].b = 0.7;
    break;

    case ORANGE:
      emitters[faceID].r = 1;
      emitters[faceID].g = 0.64;
    break;

    case YELLOW:
      emitters[faceID].r = 1;
      emitters[faceID].g = 1;
    break;

    case GREEN:
      emitters[faceID].g = 1;
    break;

    case BLUE:
      emitters[faceID].b = 1;
    break;

    case INDIGO:
      emitters[faceID].r = 0.58;
      emitters[faceID].b = 0.83;
    break;

    case VIOLET:
      emitters[faceID].r = 0.63;
      emitters[faceID].g = 0.13;
      emitters[faceID].b = 0.94;
    break;

    case BLACK:
    break;

    case WHITE:
      emitters[faceID].r = 1;
      emitters[faceID].g = 1;
      emitters[faceID].b = 1;
    break;
  }
}

/**
 * Set how many particles to use for an emitter
 */
void setEmitterParticles(int faceID, int count) {
  emitters[faceID].numberOfParticles = count;
}

/**
 * Set how fast the emitter should fire out particles
 */
void setEmitterVelocityMultiplier(int faceID, float velocity) {
  emitters[faceID].velocityMultiplier = velocity;
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

/* Arrays for current key state */
int keyStates[255]; // Create an array of boolean values of length 255 (0-254)

/* Standard key press */
void keyboardDown(int key, int x, int y) {
  if (key == 27)
    exit(0);
  else if (key == ' ') {
      paused = !paused;
      glutPostRedisplay();
  }
  else if (key == 'z') {
    axisEnabled = !axisEnabled;
  }
  else if (key == '1') {
    selectedCubeFace = 0;
  }
  else if (key == '2') {
    selectedCubeFace = 1;
  }
  else if (key == '3') {
    selectedCubeFace = 2;
  }
  else if (key == '4') {
    selectedCubeFace = 3;
  }
  else if (key == '5') {
    selectedCubeFace = 4;
  }
  else if (key == '6') {
    selectedCubeFace = 5;
  }
  else if (key == '0') {
    selectedCubeFace = -1;
  }
  else if (key == 'x') {
    printf("CameraLookYPosition: %f\n", cameraLookYPosition);
    printf("CameraLoopYAngle: %f\n", cameraLoopYAngle);
    printf("CameraLoopYPosition: %f\n", cameraLoopYPosition);
    printf("CameraDistance: %f\n", cameraDistance);
    printf("\n");
    printf("Gravity: %f\n", gravityStrength);
    printf("Bounce: %f\n", bounceCoefficient);
    printf("Particle Count: %d\n", currentParticleLimit);
    printf("Render Type: %d", currentRenderType);
    printf("Gravity Type: %d\n", currentGravityType);
    printf("Emitter Height: %2.1f\n", emitterYPosition);
    printf("\n");
    int i;
    for (i = 0; i < emitterCount; i++) {
      printf("= Emitter: %d\n", i);
      printf("Color: %1.2f %1.2f %1.2f\n", emitters[i].r, emitters[i].g, emitters[i].b);
      printf("Particles: %d\n", emitters[i].numberOfParticles);
      printf("Velocity: %d\n", emitters[i].velocityMultiplier);
      printf("\n\n");
    }
    printf("\n\n");
  } else {
    keyStates[key] = 1;
  }
}

/* Key released */
void keyboardUp(int key, int x, int y) {
  keyStates[key] = 0;
}

/* Handle standard key press */
void keyboardKeys(int key) {
  switch (key) {
    case 'w':
      cameraDistance -= 10 * deltaTime;

      if (cameraDistance < -15) cameraDistance = -15;
      break;
    case 's':
      cameraDistance += 10 * deltaTime;

      if (cameraDistance > 19) cameraDistance = 19;
      break;
    case ',':
    case '<':
      speed *= 1 - (0.5 * deltaTime);

      if(speed <= 0.01) speed = 0;
      break;
    case '.':
    case '>':
      if(speed <= 0) speed = 0.1;
      else speed *= 1 + (0.5 * deltaTime);

      if(speed > 10) speed = 10;
      break;
  }
}

/* Run all the pressed keys */
void keyboardOperations(void) {
  int i;
  for (i = 0; i < 255; i++) {
    if (keyStates[i]) {
      keyboardKeys(i);
    }
  }
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

///////////////////////////////////////////////

/**
 * Called when OpenGL is idle
 */
void idleTick(void) {
  // Calculate FPS
  calculateFPS();

  // Press any (currently down) keys
  keyboardOperations();
  keySpecialOperations();

  // If we've received no user input, then rotate
  if (!rotationKeyboardInputReceived) {
    cameraLoopYAngle += 10 * deltaTime;
  }

  // Calculate everything!
  if(!paused)
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
  emitters[id].velocityMultiplier = 1;
  emitters[id].particleLifeTime = 20;

  // Reset all particles
  int i;
  for(i = 0; i < PARTICLES_PER_EMITTER_LIMIT; i++) { //PARTICLES_PER_EMITTER_LIMIT
    emitters[id].particles[i].dead = 1;
    emitters[id].particles[i].deadTime = 0;
    emitters[id].particles[i].firstSpawn = 1;
    emitters[id].particles[i].display = 0;
    emitters[id].particles[i].lifeTime = 0;
  }

  // Set our current limit
  emitters[id].numberOfParticles = currentEmitterParticleLimit;

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
      emitters[id].numberOfParticles = currentEmitterParticleLimit / 10;
      emitters[id].spawnVelocity.y = 10;
    break;
  }

}

void createEmitters() {

  int e;
  emitterCount = 6;
  for(e = 0; e < emitterCount; e++) {
    createEmitter(e);
  }
  //emitterCount = 6;
}

/////////////////////////////////////////////////

void setGlobalBounce(int value) {
  // Hacky way of handling value
  bounceCoefficient = value / 100.0;
}

void setGravity(int value) {
  if(value == 0) {
	  gravity = 0;
	  //gravityStrength = 0;
  } else {
	  gravity = 1;
	  gravityStrength = value / 100.0;
  }

}

void setGlobalParticles(int value) {

  currentEmitterParticleLimit = value / emitterCount;
  if(currentEmitterParticleLimit > PARTICLES_PER_EMITTER_LIMIT)
     currentEmitterParticleLimit = PARTICLES_PER_EMITTER_LIMIT;

  // Reset all emitters with new limit
  createEmitters();

  currentParticleLimit = 0;
  int i;
  for(i = 0; i < emitterCount; i++) {
	  emitters[i].numberOfParticles = currentEmitterParticleLimit;
	  currentParticleLimit += emitters[i].numberOfParticles;
  }
}

void setGlobalGravityType(int type) {
  currentGravityType = type;

  // Reset to new gravity
  createEmitters();
}

void setGlobalDrawType(int type) {
  currentRenderType = type;

  // For lines we must be using Verlet
  if(type == LINES) {
    if(currentGravityType != VERLET_APPROXIMATION) {
      setGlobalGravityType(VERLET_APPROXIMATION);
    }
  }
}

/////////

void selectLocalLifetime(int lifetime) {

  if(selectedCubeFace < 0) {
	// Call this function for each face
	int id;
	for (id = 0; id < EMITTER_LIMIT; id ++) {
	  emitters[id].particleLifeTime = lifetime;
	}
  } else {
	  emitters[selectedCubeFace].particleLifeTime = lifetime;
  }
}

void selectLocalColorItem(int color) {

  if(selectedCubeFace < 0) {
    // Call this function for each face
    int id;
    for (id = 0; id < EMITTER_LIMIT; id ++) {
      setEmitterColor(id, color);
    }
  } else {
    setEmitterColor(selectedCubeFace, color);
  }
}

void selectLocalParticleItem(int count) {

  if(selectedCubeFace < 0) {
    // Call this function for each face
    int id;
    for (id = 0; id < EMITTER_LIMIT; id ++) {
      setEmitterParticles(id, count);
    }
  } else {
    setEmitterParticles(selectedCubeFace, count);
  }
}

void selectLocalVelocityItem(int velocity) {

  float actualVelocity = velocity / 4;

  if(selectedCubeFace < 0) {
    // Call this function for each face
    int id;
    for (id = 0; id < EMITTER_LIMIT; id ++) {
      setEmitterVelocityMultiplier(id, actualVelocity);
    }
  } else {
    setEmitterVelocityMultiplier(selectedCubeFace, actualVelocity);
  }
}

///////////////////////////////////////////////

void selectLocalMenuItem() {
  return;
}

void selectMainMenuItem(int item) {
  if(item == TOGGLE_FLOOR) {
	  drawFloor = !drawFloor;
	  createEmitters();
  }
  return;
}

///////////////////////////////////////////////

void selectPreset(int preset) {
  switch (preset) {
    case WATERFALL:
      createEmitters();
      currentGravityType = REAL_GRAVITY;
      currentRenderType = BILLBOARD;
      setGravity(-980);
      setGlobalBounce(10);

      selectedCubeFace = -1;
      selectLocalColorItem(BLUE);
      selectLocalParticleItem(0);

      selectedCubeFace = 3;
      selectLocalParticleItem(100000);

      rotationKeyboardInputReceived = 1;
      cameraLookYPosition = 0.660000;
      cameraLoopYAngle = 89.940414;
      cameraLoopYPosition = -7.890000;
      cameraDistance = -2.66;

      break;

    case FIRE_EMITTER:
      setGlobalGravityType(VERLET_APPROXIMATION);
      setGlobalDrawType(LINES);

      selectedCubeFace = -1;
      selectLocalColorItem(RED);
      selectLocalParticleItem(0);

      selectedCubeFace = 5;
      selectLocalParticleItem(100000);

      emitterYPosition = -11;

      rotationKeyboardInputReceived = 0;

      break;

    case DEFAULT:
      createEmitters();
      break;
  }
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
  glutKeyboardFunc(keyboardDown);
  glutKeyboardUpFunc(keyboardUp);
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

  // Enable color blend for alpha
  glEnable (GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

  // Accept ift closer to the camera
  glDepthFunc(GL_LESS);

  // Smooth points
  glEnable(GL_POINT_SMOOTH);

  // Global Bounce
  int globalBounceMenu = glutCreateMenu( setGlobalBounce );
  glutAddMenuEntry("Very Low", 10 );
  glutAddMenuEntry("Low", 50 );
  glutAddMenuEntry("Default", 60 );
  glutAddMenuEntry("High", 75 );
  glutAddMenuEntry("Very High", 90 );

  // Global Gravity
  int globalGravityMenu = glutCreateMenu( setGravity );
  glutAddMenuEntry("High Anti-Gravity", 1200 );
  glutAddMenuEntry("Anti-Gravity", 980 );
  glutAddMenuEntry("Low Anti-Gravity", 300 );
  glutAddMenuEntry("Off", 0);
  glutAddMenuEntry("Very Low", -100 );
  glutAddMenuEntry("Low", -300 );
  glutAddMenuEntry("Default", -980 );
  glutAddMenuEntry("High", -1200 );
  glutAddMenuEntry("Very High", -1500 );

  // Global Particle Count
  int globalParticleMenu = glutCreateMenu( setGlobalParticles );
  glutAddMenuEntry("Low", 2000 );
  glutAddMenuEntry("Medium", 3000 );
  glutAddMenuEntry("Default", 6000 );
  glutAddMenuEntry("High", 50000 );
  glutAddMenuEntry("Very High", 100000 );
  glutAddMenuEntry("Extreme", 600000 );
  glutAddMenuEntry("Incredible", 990000 ); // maximum is 165000 per emitter

  // Gravity type
  int globalGravityTypeMenu = glutCreateMenu( setGlobalGravityType );
  glutAddMenuEntry("Euler Simulation", REAL_GRAVITY );
  glutAddMenuEntry("Verlet Approximation", VERLET_APPROXIMATION );

  // Draw type
  int globalDrawTypeMenu = glutCreateMenu( setGlobalDrawType );
  glutAddMenuEntry("Points", POINT );
  glutAddMenuEntry("Jitter", JITTER_POINTS );
  glutAddMenuEntry("Lines", LINES );
  glutAddMenuEntry("Billboard", BILLBOARD );

  // Global Settings
  int globalMenu = glutCreateMenu( selectMainMenuItem );
  glutAddSubMenu("Set Bounce", globalBounceMenu );
  glutAddSubMenu("Set Gravity", globalGravityMenu );
  glutAddSubMenu("Set Particles", globalParticleMenu );
  glutAddSubMenu("Set Simulation Type", globalGravityTypeMenu );
  glutAddSubMenu("Set Draw Type", globalDrawTypeMenu );
  glutAddMenuEntry("Toggle Floor", TOGGLE_FLOOR );

  /////////////

  // Color
  int localColorMenu = glutCreateMenu( selectLocalColorItem );
  glutAddMenuEntry("Red", RED );
  glutAddMenuEntry("Pink", PINK );
  glutAddMenuEntry("Orange", ORANGE );
  glutAddMenuEntry("Yellow", YELLOW );
  glutAddMenuEntry("Green", GREEN );
  glutAddMenuEntry("Blue", BLUE );
  glutAddMenuEntry("Indigo", INDIGO );
  glutAddMenuEntry("Violet", VIOLET );
  glutAddMenuEntry("While", WHITE );
  glutAddMenuEntry("Black", BLACK );

  // Color
  int localParticleMenu = glutCreateMenu( selectLocalParticleItem );
  glutAddMenuEntry("0", 0 );
  glutAddMenuEntry("500", 500 );
  glutAddMenuEntry("1,000", 1000 );
  glutAddMenuEntry("5,000", 5000 );
  glutAddMenuEntry("10,000", 10000 );
  glutAddMenuEntry("50,000", 50000 );
  glutAddMenuEntry("100,000", 100000 );
  glutAddMenuEntry("Max", PARTICLES_PER_EMITTER_LIMIT );

  // Velocity
  int localVelocityMenu = glutCreateMenu( selectLocalVelocityItem );
  glutAddMenuEntry("Very Low", 1 );
  glutAddMenuEntry("Low", 2 );
  glutAddMenuEntry("Medium", 4 );
  glutAddMenuEntry("High", 8 );
  glutAddMenuEntry("Very High", 16 );

  // Lifetime
  int localLifetimeMenu = glutCreateMenu( selectLocalLifetime );
  glutAddMenuEntry("1 Second", 1 );
  glutAddMenuEntry("5 Seconds", 5 );
  glutAddMenuEntry("10 Seconds", 10 );
  glutAddMenuEntry("20 Seconds", 20 );
  glutAddMenuEntry("30 Seconds", 30 );
  glutAddMenuEntry("60 Seconds", 60 );


  // Local Settings
  int localMenu = glutCreateMenu( selectMainMenuItem );
  glutAddSubMenu("Set Color", localColorMenu );
  glutAddSubMenu("Set Particles", localParticleMenu );
  glutAddSubMenu("Set Velocity", localVelocityMenu );
  glutAddSubMenu("Set Lifetime", localLifetimeMenu );


  //////////////

  int globalPresetMenu = glutCreateMenu( selectPreset );
  glutAddMenuEntry("Waterfall", WATERFALL );
  glutAddMenuEntry("Volcano", FIRE_EMITTER );
  //glutAddMenuEntry("Lines", LINES );
  //glutAddMenuEntry("Billboard", BILLBOARD );

  // Create a menu
  glutCreateMenu( selectMainMenuItem );

  // Add menu items
  glutAddSubMenu("Global Settings", globalMenu);
  glutAddSubMenu("Selected Face Settings", localMenu);
  glutAddSubMenu("Preset", globalPresetMenu );

  // Associate a mouse button with menu
  glutAttachMenu(GLUT_RIGHT_BUTTON);

  // Draw (and save on the GPU) the axes and floor
  makeAxes();
  makeGridFloor(WORLD_SIZE, FLOOR_HEIGHT);
  makeCeiling(WORLD_SIZE, CEILING_HEIGHT);
}

/////////////////////////////////////////////////

int main(int argc, char *argv[]) {
  // Seed random
  srand(time(NULL));

  createEmitters();

  // Boot the graphics engine
  initGraphics(argc, argv);
  glutMainLoop();

  return 0;
}
