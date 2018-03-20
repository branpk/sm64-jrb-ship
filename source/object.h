#ifndef OBJECT_H
#define OBJECT_H


#include "util.h"


typedef struct Object Object;


struct Object {
  v3f scale;
  v3f pos;
  v3f vel;
  v3i displayAngle;
  v3i platformRotation;
  s16 *collisionModel;
  Mtxf v21C;

  s32 v0F4;
  s32 v0F8;
};


void applyPlatformDisplacement(v3f *p, v3h *marioFaceAngle, Object *plat);


void initJrbShipAfloat(Object *o);
void updateJrbShipAfloat(Object *curObj);
void updateJrbShipAfloatIndex(Object *curObj, s32 idx);


#endif