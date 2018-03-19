#include "object.h"
#include "surface.h"
#include "util.h"

#include <GLFW/glfw3.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>


Object shipInst;
Object *ship = &shipInst;


struct {
  v3f pos;
  f32 pitch;
  f32 yaw;
} camera = {{3080, 820, 2375}, 0, 3.14159f / 2.0f};


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


void render(GLFWwindow *window) {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glLoadIdentity();
  glFrustum(-1, 1, -1, 1, 2, 10000);
  glRotatef(-camera.pitch * 180.0f / 3.1415926f, 1, 0, 0);
  glRotatef(-camera.yaw * 180.0f / 3.1415926f, 0, 1, 0);
  glRotatef(180, 0, 1, 0);
  glTranslatef(-camera.pos.x, -camera.pos.y, -camera.pos.z);

  for (int i = 0; i < surfacesAllocated; i++) {
    Surface *s = &surfacePool[i];

    if (s->normal.y > 0.01)
      glColor4f(0.5f, 0.5f, 1, 1);
    else if (s->normal.y < -0.01)
      glColor4f(1, 0.5f, 0.5f, 1);
    else
      glColor4f(0.3f, 0.8f, 0.3f, 1);

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

  glfwSwapBuffers(window);
  glfwPollEvents();
}


int main(void) {
  initJrbShipAfloat(ship);
  initStaticPartition();

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
