#include "object.h"
#include "surface.h"
#include "util.h"

#include <GLFW/glfw3.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct {
  s16 x0;
  s16 z0;
  s16 x1;
  s16 z1;
  f32 *y;
} SurfaceHeightMap;

#define map_get(m, x, z) m->y[(m->x1 - m->x0 + 1) * ((z) - m->z0) + (x) - m->x0]
#define map_none -12000.0f

void initSurfaceHeightMap(SurfaceHeightMap *m, Surface *s) {
  char classif = classifySurface(s);
  if (classif == 'w') {
    m->x0 = 0;
    m->x1 = -1;
    m->z0 = 0;
    m->z1 = -1;
    m->y = (f32 *) malloc(sizeof(f32));
    if (m->y == NULL) {
      fprintf(stderr, "Out of memory\n");
      exit(1);
    }
    return;
  }

  m->x0 = min3(s->vertex1.x, s->vertex2.x, s->vertex3.x) - 3;
  m->x1 = max3(s->vertex1.x, s->vertex2.x, s->vertex3.x) + 3;
  m->z0 = min3(s->vertex1.z, s->vertex2.z, s->vertex3.z) - 3;
  m->z1 = max3(s->vertex1.z, s->vertex2.z, s->vertex3.z) + 3;

  m->y = (f32 *)
    malloc((m->x1 - m->x0 + 1) * (m->z1 - m->z0 + 1) * sizeof(f32));
  if (m->y == NULL) {
    fprintf(stderr, "Out of memory\n");
    exit(1);
  }

  for (s32 x = m->x0; x <= m->x1; x++) {
    for (s32 z = m->z0; z <= m->z1; z++) {
      bool onSurface;

      if (classif == 'f')
        onSurface = getFloorHeight(s, x, z, &map_get(m, x, z));
      else
        onSurface = getCeilHeight(s, x, z, &map_get(m, x, z));

      if (!onSurface)
        map_get(m, x, z) = map_none;
    }
  }
}

SurfaceHeightMap *buildHeightMaps(void) {
  SurfaceHeightMap *maps = (SurfaceHeightMap *)
    malloc((surfacesAllocated + 1) * sizeof(SurfaceHeightMap));
  if (maps == NULL) {
    fprintf(stderr, "Out of memory\n");
    exit(1);
  }

  for (int i = 0; i < surfacesAllocated; i++)
    initSurfaceHeightMap(&maps[i], &surfacePool[i]);
  maps[surfacesAllocated].y = NULL;

  return maps;
}

void freeHeightMaps(SurfaceHeightMap *s) {
  for (SurfaceHeightMap *m = s; m->y != NULL; m++)
    free(m->y);
  free(s);
}


Object shipInst;
Object *ship = &shipInst;


typedef struct SpotNode SpotNode;

struct SpotNode {
  s16 x;
  s16 z;
  f32 y;
  SpotNode *next;
};


SpotNode *spotsByIndex[0x100];


SpotNode *findVolatileSpotsForSurface(Surface *s, SurfaceHeightMap *m0) {
  if (classifySurface(s) != 'f') return NULL;
  if (s->object == NULL) return NULL;

  SpotNode *spots = NULL;

  for (s16 x = m0->x0; x <= m0->x1; x++) {
    for (s16 z = m0->z0; z <= m0->z1; z++) {
      f32 y0 = map_get(m0, x, z);
      if (y0 == map_none) continue;

      v3f ps[] = {
        {x+0.05f, y0, z+0.05f},
        {x+0.95f, y0, z+0.05f},
        {x+0.05f, y0, z+0.95f},
        {x+0.95f, y0, z+0.95f},
      };

      int numVol = 0;

      for (int i = 0; i < 4; i++) {
        applyPlatformDisplacement(&ps[i], NULL, s->object);

        Surface *floor;
        f32 fh = findFloor(ps[i], &floor);

        if (ps[i].y > fh + 100.0f)
          numVol += 1;
      }

      if (numVol > 0) {
        SpotNode *spot = (SpotNode *) malloc(sizeof(SpotNode));
        spot->x = x;
        spot->z = z;
        spot->y = y0;
        spot->next = spots;
        spots = spot;
      }
    }
  }

  return spots;
}


SpotNode *findVolatileSpots(s32 index) {
  initDynamicPartition();
  updateJrbShipAfloatIndex(ship, index);
  loadObjectCollisionModel(ship);

  SurfaceHeightMap *maps = buildHeightMaps();

  initDynamicPartition();
  updateJrbShipAfloat(ship);
  loadObjectCollisionModel(ship);

  SpotNode *spots = NULL;

  for (int i = 0; i < surfacesAllocated; i++) {
    Surface *s = &surfacePool[i];
    if (classifySurface(s) != 'f') continue;
    SurfaceHeightMap *m0 = &maps[i];

    SpotNode *surfSpots = findVolatileSpotsForSurface(s, m0);

    if (surfSpots != NULL) {
      SpotNode *sn = surfSpots;
      while (sn->next != NULL)
        sn = sn->next;
      sn->next = spots;
      spots = surfSpots;
    }
  }

  freeHeightMaps(maps);
  return spots;
}


void computeAllVolatileSpots(void) {
  printf("Computing volatile spots\n");
  for (int idx = 0; idx < 0x100; idx++) {
    SpotNode *spots = findVolatileSpots(idx);
    spotsByIndex[idx] = spots;

    int count = 0;
    while (spots != NULL) {
      count++;
      spots = spots->next;
    }
    printf("Index %d: %d\n", idx, count);
  }
}


struct {
  v3f pos;
  f32 pitch;
  f32 yaw;
} camera = {{3080, 2520, 2375}, 0, 3.14159f / 2.0f};


GLFWwindow *openWindow(void) {
  glfwInit();

  GLFWwindow *window =
    glfwCreateWindow(480, 480, "JRB Ship Afloat", NULL, NULL);
  glfwMakeContextCurrent(window);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  return window;
}


void updateCamera(GLFWwindow *window) {
  f32 forward = 0;
  f32 sideways = 0;
  f32 upward = 0;

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) forward += 1;
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) forward -= 1;
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) sideways += 1;
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) sideways -= 1;
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) upward += 1;
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) upward -= 1;

  f32 mag = sqrtf(forward*forward + sideways*sideways + upward*upward);
  if (mag != 0) {
    forward /= mag;
    sideways /= mag;
    upward /= mag;

    f32 speed = 35.0f;
    forward *= speed;
    sideways *= speed;
    upward *= speed;

    camera.pos.x += forward * sinf(camera.yaw);
    camera.pos.z += forward * cosf(camera.yaw);

    camera.pos.x += sideways * sinf(camera.yaw - 3.1415926f / 2.0f);
    camera.pos.z += sideways * cosf(camera.yaw - 3.1415926f / 2.0f);

    camera.pos.y += upward;
  }

  f32 yaw = 0;
  f32 pitch = 0;

  if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) yaw += 1;
  if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) yaw -= 1;
  if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) pitch += 1;
  if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) pitch -= 1;

  camera.yaw += yaw * 0.03f;
  camera.pitch += pitch * 0.03f;
}


void renderShipSurfaces(void) {
  for (int i = 0; i < surfacesAllocated; i++) {
    Surface *s = &surfacePool[i];

    switch (classifySurface(s)) {
    case 'f': glColor4f(0.5f, 0.5f, 1, 1); break;
    case 'c': glColor4f(1, 0.5f, 0.5f, 1); break;
    case 'w': glColor4f(0.3f, 0.8f, 0.3f, 1); break;
    }

    glBegin(GL_TRIANGLES);
    glVertex3f(s->vertex1.x, s->vertex1.y, s->vertex1.z);
    glVertex3f(s->vertex2.x, s->vertex2.y, s->vertex2.z);
    glVertex3f(s->vertex3.x, s->vertex3.y, s->vertex3.z);
    glEnd();

    glColor3f(0, 0, 0);
    glBegin(GL_LINE_LOOP);
    glVertex3f(s->vertex1.x, s->vertex1.y, s->vertex1.z);
    glVertex3f(s->vertex2.x, s->vertex2.y, s->vertex2.z);
    glVertex3f(s->vertex3.x, s->vertex3.y, s->vertex3.z);
    glEnd();
  }
}


void renderVolatileSpots(void) {
  int index = ((u32) ship->v0F4 / 0x100) % 0x100;
  SpotNode *spots = spotsByIndex[index];
  if (spots == NULL) return;

  glColor3f(1, 1, 1);
  while (spots != NULL) {
    glBegin(GL_TRIANGLE_STRIP);
    glVertex3f(spots->x, spots->y, spots->z);
    glVertex3f(spots->x + 1, spots->y, spots->z);
    glVertex3f(spots->x, spots->y, spots->z + 1);
    glVertex3f(spots->x + 1, spots->y, spots->z + 1);
    glEnd();

    spots = spots->next;
  }
}


void render(GLFWwindow *window) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glLoadIdentity();
  glFrustum(-1, 1, -1, 1, 2, 10000);
  glRotatef(-camera.pitch * 180.0f / 3.1415926f, 1, 0, 0);
  glRotatef(-camera.yaw * 180.0f / 3.1415926f, 0, 1, 0);
  glRotatef(180, 0, 1, 0);
  glTranslatef(-camera.pos.x, -camera.pos.y, -camera.pos.z);

  renderShipSurfaces();
  renderVolatileSpots();

  glfwSwapBuffers(window);
  glfwPollEvents();
}


int main(void) {
  initJrbShipAfloat(ship);
  initStaticPartition();
  computeAllVolatileSpots();  

  GLFWwindow *window = openWindow();

  double accumTime = 0;
  double lastTime = glfwGetTime();

  while (!glfwWindowShouldClose(window)) {
    double currentTime = glfwGetTime();
    accumTime += currentTime - lastTime;
    lastTime = currentTime;
    while (accumTime >= 1.0/30) {
      initDynamicPartition();

      updateJrbShipAfloat(ship);
      loadObjectCollisionModel(ship);

      updateCamera(window);

      accumTime -= 1.0/30;
    }

    render(window);
  }

  glfwTerminate();
  return 0;
}
