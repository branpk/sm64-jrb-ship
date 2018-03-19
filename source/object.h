#ifndef OBJECT_H
#define OBJECT_H


#include "util.h"


typedef struct Object Object;


struct Object {
  v3f scale;
  v3f pos;
  v3i displayAngle;
  s16 *collisionModel;
  Mtxf v21C;
};


#endif