#ifndef SURFACE_H
#define SURFACE_H


#include "object.h"
#include "util.h"


typedef struct {
  s16 type;
  s16 v02;
  s8 v04;
  s8 v05;
  s16 lowerY;
  s16 upperY;
  v3h vertex1;
  v3h vertex2;
  v3h vertex3;
  v3f normal;
  f32 originOffset;
  Object *object;
} Surface;


typedef struct SurfaceNode {
  struct SurfaceNode *tail;
  Surface *head;
} SurfaceNode;


typedef struct {
  union {
    struct {
      SurfaceNode *floors;
      Surface *nullFloorHead;
      SurfaceNode *ceils;
      Surface *nullCeilHead;
      SurfaceNode *walls;
      Surface *nullWallHead;
    };    
    SurfaceNode lists[3];
  };
} SpatialPartitionCell;


typedef struct {
  v3f pos;
  f32 offsetY;
  f32 radius;
  s16 numSurfaces;
  Surface *surfaces[4];
} CollisionData;


extern SpatialPartitionCell staticPartition[16 * 16];
extern SpatialPartitionCell dynamicPartition[16 * 16];

extern SurfaceNode *surfaceNodePool;
extern Surface *surfacePool;

extern s32 surfaceNodesAllocated;
extern s32 surfacesAllocated;
extern s32 numStaticSurfaceNodes;
extern s32 numStaticSurfaces;


Surface *allocSurface(void);
void initStaticPartition(void);
void addSurface(Surface *tri, bool dynamic);
void initDynamicPartition(void);
void loadObjectCollisionModel(Object *curObj);

f32 findFloor(v3f pos, Surface **pfloor);
f32 findCeil(v3f pos, Surface **pceil);
s32 findWallCols(CollisionData *data);

char classifySurface(Surface *s);
bool getFloorHeight(Surface *tri, s16 x, s16 z, f32 *pheight);
bool getCeilHeight(Surface *tri, s16 x, s16 z, f32 *pheight);


#endif