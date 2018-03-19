#include "surface.h"

#include "object.h"
#include "util.h"

#include <math.h>
#include <stdlib.h>


static SurfaceNode surfaceNodePoolI[7000];
static Surface surfacePoolI[2300];

SpatialPartitionCell staticPartition[16 * 16];
SpatialPartitionCell dynamicPartition[16 * 16];

SurfaceNode *surfaceNodePool = &surfaceNodePoolI;
Surface *surfacePool = &surfacePoolI;

s32 surfaceNodesAllocated;
s32 surfacesAllocated;
s32 numStaticSurfaceNodes;
s32 numStaticSurfaces;


/** 80382490(J) */
SurfaceNode *allocSurfaceNode(void) {
  SurfaceNode *node = &surfaceNodePool[surfaceNodesAllocated++];
  node->tail = NULL;
  return node;
}


/** 803824F8(J) */
Surface *allocSurface(void) {
  Surface *tri = &surfacePool[surfacesAllocated++];
  tri->type = 0;
  tri->v02 = 0;
  tri->v04 = 0;
  tri->v05 = 0;
  tri->object = NULL;
  return tri;
}


/** 80382590(J) */
void initSpatialPartition(SpatialPartitionCell *partition) {
  for (s32 i = 0; i < 16 * 16; i++) {
    partition[i].floors = NULL;
    partition[i].ceils = NULL;
    partition[i].walls = NULL;
  }
}


/** 803825D0(J) */
void initStaticPartition(void) {
  initSpatialPartition(&staticPartition);
}


/** 803825FC(J) */
void addSurfaceToPartition(
  bool dynamic, s16 xidx, s16 zidx, Surface *tri)
{
  SurfaceNode *newNode = allocSurfaceNode();

  s16 listIdx;
  s16 sortDir;
  
  if (tri->normal.y > 0.01) {
    listIdx = 0; // floors
    sortDir = 1; // highest to lowest, then insertion order
  }
  else if (tri->normal.y < -0.01) {
    listIdx = 1; // ceils
    sortDir = -1; // lowest to highest, then insertion order
  }
  else {
    listIdx = 2; // walls
    sortDir = 0; // insertion order
  
    if (tri->normal.x < -0.707 || tri->normal.x > 0.707)
      tri->v04 |= 0x08;
  }

  //! Sorting by vertex1.y - lots of cases where this is incorrect
  s16 triPriority = tri->vertex1.y * sortDir;
  newNode->head = tri;
  
  SurfaceNode *list;
  if (dynamic)
    list = &dynamicPartition[16 * zidx + xidx].lists[listIdx];
  else
    list = &staticPartition[16 * zidx + xidx].lists[listIdx];

  while (list->tail != NULL) {
    s16 priority = list->tail->head->vertex1.y * sortDir;
    if (triPriority > priority) break;
    list = list->tail;
  }

  newNode->tail = list->tail;
  list->tail = newNode;
}


/** 8038283C(J) */
s16 min3(s16 t1, s16 t2, s16 t3) {
  if (t2 < t1) t1 = t2;
  if (t3 < t1) t1 = t3;
  return t1;
}


/** 8038289C(J) */
s16 max3(s16 t1, s16 t2, s16 t3) {
  if (t2 > t1) t1 = t2;
  if (t3 > t1) t1 = t3;
  return t1;
}


/** 803828FC(J) */
s16 lowerPartitionCellIdx(s16 t) {
  t += 0x2000;
  if (t < 0) t = 0;

  s16 idx = t / 0x400;

  // Include extra cell if close to boundary
  if ((t & 0x03FF) < 50)
    idx -= 1;
  
  if (idx < 0) idx = 0;

  // Potentially > 15, but since the upper index is <= 15, not exploitable
  return idx;
}


/** 80382990(J) */
s16 upperPartitionCellIdx(s16 t) {
  t += 0x2000;
  if (t < 0) t = 0;

  s16 idx = t / 0x400;

  // Include extra cell if close to boundary
  if ((t & 0x03FF) >= 0x03CF)
    idx += 1;

  if (idx > 15) idx = 15;

  // Potentially < 0, but since lower index is >= 0, not exploitable
  return idx;
}


/** 80382A2C(J) */
void addSurface(Surface *tri, bool dynamic) {
  s16 minX = min3(tri->vertex1.x, tri->vertex2.x, tri->vertex3.x);
  s16 minZ = min3(tri->vertex1.z, tri->vertex2.z, tri->vertex3.z);
  s16 maxX = max3(tri->vertex1.x, tri->vertex2.x, tri->vertex3.x);
  s16 maxZ = max3(tri->vertex1.z, tri->vertex2.z, tri->vertex3.z);
  
  s16 xidx0 = lowerPartitionCellIdx(minX);
  s16 xidx1 = upperPartitionCellIdx(maxX);
  s16 zidx0 = lowerPartitionCellIdx(minZ);
  s16 zidx1 = upperPartitionCellIdx(maxZ);

  for (s16 zidx = zidx0; zidx <= zidx1; zidx++)
    for (s16 xidx = xidx0; xidx <= xidx1; xidx++)
      addSurfaceToPartition(dynamic, xidx, zidx, tri);
}


/** 80382B7C(J) */
Surface *readSurfaceData(s16 *vertexData, s16 **arg1) {
  s16 offset1 = 3 * *(*arg1 + 0);
  s16 offset2 = 3 * *(*arg1 + 1);
  s16 offset3 = 3 * *(*arg1 + 2);

  s32 x1 = *(vertexData + offset1 + 0);
  s32 y1 = *(vertexData + offset1 + 1);
  s32 z1 = *(vertexData + offset1 + 2);
  
  s32 x2 = *(vertexData + offset2 + 0);
  s32 y2 = *(vertexData + offset2 + 1);
  s32 z2 = *(vertexData + offset2 + 2);
  
  s32 x3 = *(vertexData + offset3 + 0);
  s32 y3 = *(vertexData + offset3 + 1);
  s32 z3 = *(vertexData + offset3 + 2);
  
  // clockwise
  f32 nx = (y2 - y1) * (z3 - z2) - (z2 - z1) * (y3 - y2);
  f32 ny = (z2 - z1) * (x3 - x2) - (x2 - x1) * (z3 - z2);
  f32 nz = (x2 - x1) * (y3 - y2) - (y2 - y1) * (x3 - x2);
  f32 mag = sqrtf(nx*nx + ny*ny + nz*nz);

  s32 minY = y1;
  if (y2 < minY) minY = y2;
  if (y3 < minY) minY = y3;
  
  s32 maxY = y1;
  if (y2 > maxY) maxY = y2;
  if (y3 > maxY) maxY = y3;

  if (mag < 0.0001) return NULL;
  mag = (f32) (1.0 / mag);
  nx *= mag;
  ny *= mag;
  nz *= mag;

  Surface *tri = allocSurface();
  
  tri->vertex1.x = x1;
  tri->vertex1.y = y1;
  tri->vertex1.z = z1;

  tri->vertex2.x = x2;
  tri->vertex2.y = y2;
  tri->vertex2.z = z2;

  tri->vertex3.x = x3;
  tri->vertex3.y = y3;
  tri->vertex3.z = z3;
  
  tri->normal.x = nx;
  tri->normal.y = ny;
  tri->normal.z = nz;
  
  tri->originOffset = -(x1*nx + y1*ny + z1*nz);

  tri->lowerY = minY - 5;
  tri->upperY = maxY + 5;
  
  return tri;
}


/** 80383068(J) */
void readStaticSurfaces(
  s16 **data, s16 *vertexData, s16 surfaceType, s8 **arg3)
{
  u8 valB = 0;
  u16 val8 = 0; //p80382F84(surfaceType);
  u16 val6 = 0; //p80382FEC(surfaceType);
  
  s32 numTris = *(*data)++;

  for (s32 i = 0; i < numTris; i++) {
    if (*arg3 != NULL)
      valB = *(*arg3)++;

    Surface *tri = readSurfaceData(vertexData, data);
    if (tri != NULL) {
      tri->v05 = valB;
      tri->type = surfaceType;
      tri->v04 = (s8) val6;

      if (val8 != 0)
        tri->v02 = *(*data + 3);
      else
        tri->v02 = 0;

      addSurface(tri, false);
    }

    *data += 3;
    if (val8 != 0)
      *data += 1;
  }
}


/** 803831D0(J) */
s16 *readVertexData(s16 **data) {
  s16 numVerts = *(*data)++;
  s16 *vertexData = *data;
  *data += 3 * numVerts;
  return vertexData;
}


/** 803835A4(J) */
void initDynamicPartition(void) {
  surfacesAllocated = numStaticSurfaces;
  surfaceNodesAllocated = numStaticSurfaceNodes;
  initSpatialPartition(&dynamicPartition);
}


/** 8029D62C(J) */
void applyObjectScale(Object *o, Mtxfp dst, Mtxfp src) {
  dst[0][0] = src[0][0] * o->scale.x;
  dst[1][0] = src[1][0] * o->scale.y;
  dst[2][0] = src[2][0] * o->scale.z;
  dst[3][0] = src[3][0];
  
  dst[0][1] = src[0][1] * o->scale.x;
  dst[1][1] = src[1][1] * o->scale.y;
  dst[2][1] = src[2][1] * o->scale.z;
  dst[3][1] = src[3][1];
  
  dst[0][2] = src[0][2] * o->scale.x;
  dst[1][2] = src[1][2] * o->scale.y;
  dst[2][2] = src[2][2] * o->scale.z;
  dst[3][2] = src[3][2];
  
  dst[0][3] = src[0][3];
  dst[1][3] = src[1][3];
  dst[2][3] = src[2][3];
  dst[3][3] = src[3][3];
}


/** 80383614(J) */
void readObjectCollisionVertices(Object *curObj, s16 **data, s16 *vertexData) {
  s16 numVerts = *(*data)++;
  Mtxfp transform = &curObj->v21C;

  // if (curObj->g.v050 == NULL) {
  //   curObj->g.v050 = transform;
  //   buildObjectTransform(curObj, 0x06, 0x12); // pos - 0x0A0, rot - 0x0D0
  // }
  v3h rot = {
    (s16) curObj->displayAngle.pitch,
    (s16) curObj->displayAngle.yaw,
    (s16) curObj->displayAngle.roll,
  };
  matrixFromTransAndRot(curObj, &curObj->pos, &rot);

  Mtxf m;
  applyObjectScale(curObj, &m, transform);
  
  while (numVerts-- != 0) {
    s16 vx = *(*data)++;
    s16 vy = *(*data)++;
    s16 vz = *(*data)++;
    
    *vertexData++ = (s16) (m[0][0]*vx + m[1][0]*vy + m[2][0]*vz + m[3][0]);
    *vertexData++ = (s16) (m[0][1]*vx + m[1][1]*vy + m[2][1]*vz + m[3][1]);
    *vertexData++ = (s16) (m[0][2]*vx + m[1][2]*vy + m[2][2]*vz + m[3][2]);
  }
}


/** 80383828(J) */
void loadObjColModelFromVertexData(
  Object *curObj, s16 **data, s16 *vertexData)
{
  s16 surfaceType = *(*data)++;
  s32 numTris = *(*data)++;

  u16 valA = 0; //p80382F84(surfaceType);
  u16 val8 = 0; //p80382FEC(surfaceType);
  val8 |= 0x0001;

  u16 val6;
  // if (curObj->script == segmentedToVirtual(0x13001C34))
  //   val6 = 5;
  // else
    val6 = 0;
  
  for (s32 i = 0; i < numTris; i++) {
    Surface *tri = readSurfaceData(vertexData, data);

    if (tri != NULL) {
      tri->object = curObj;
      tri->type = surfaceType;

      if (valA != 0)
        tri->v02 = *(*data + 3);
      else
        tri->v02 = 0;
      
      tri->v04 |= (s8) val8;
      tri->v05 = (s8) val6;
      addSurface(tri, true);
    }

    if (valA != 0)
      *data += 4;
    else
      *data += 3;
  }
}


/** 803839CC(J) */
void loadObjectCollisionModel(Object *curObj) {
  s16 vertexData[600];

  s16 *val8 = curObj->collisionModel;
  // f32 marioDist = curObj->distToMario;
  // f32 tangibleDist = curObj->tangibleDist;

  // if (curObj->distToMario == v8038BC80)
  //   marioDist = objDistance(curObj, marioObj);

  // if (curObj->tangibleDist > 4000.0f)
  //   curObj->drawDist = curObj->tangibleDist;

  // if (!(v8033C110 & 0x00000040) && marioDist < tangibleDist &&
  //   !(curObj->v074 & 0x0008))
  {
    val8++;
    readObjectCollisionVertices(curObj, &val8, &vertexData);

    while (*val8 != 0x41) {
      loadObjColModelFromVertexData(curObj, &val8, &vertexData);
    }
  }

  // if (marioDist < curObj->drawDist)
  //   curObj->g.graphicsFlags |= 0x0001;
  // else
  //   curObj->g.graphicsFlags &= ~0x0001;
}


/** 80380690(J) */
s32 findWallColsFromList(SurfaceNode *triangles, CollisionData *data) {
  s32 numCols = 0;

  f32 x = data->pos.x;
  f32 y = data->pos.y + data->offsetY;
  f32 z = data->pos.z;

  f32 radius = data->radius;
  if (radius > 200.0f) radius = 200.0;

  while (triangles != NULL) {
    Surface *tri = triangles->head;
    triangles = triangles->tail;

    if (y < tri->lowerY || y > tri->upperY)
      continue;

    f32 nx = tri->normal.x;
    f32 ny = tri->normal.y;
    f32 nz = tri->normal.z;
    f32 offset = nx * x + ny * y + nz * z + tri->originOffset;

    if (offset < -radius || offset > radius) continue;

    f32 y1 = tri->vertex1.y;
    f32 y2 = tri->vertex2.y;
    f32 y3 = tri->vertex3.y;

    if (tri->v04 & 0x08) {
      f32 z1 = -tri->vertex1.z;
      f32 z2 = -tri->vertex2.z;
      f32 z3 = -tri->vertex3.z;
      
      if (tri->normal.x > 0.0f) {
        if ((y1 - y) * (z2 - z1) - (z1 - -z) * (y2 - y1) > 0.0f) continue;
        if ((y2 - y) * (z3 - z2) - (z2 - -z) * (y3 - y2) > 0.0f) continue;
        if ((y3 - y) * (z1 - z3) - (z3 - -z) * (y1 - y3) > 0.0f) continue;
      }
      else {
        if ((y1 - y) * (z2 - z1) - (z1 - -z) * (y2 - y1) < 0.0f) continue;
        if ((y2 - y) * (z3 - z2) - (z2 - -z) * (y3 - y2) < 0.0f) continue;
        if ((y3 - y) * (z1 - z3) - (z3 - -z) * (y1 - y3) < 0.0f) continue;
      }
    }
    else {
      f32 x1 = tri->vertex1.x;
      f32 x2 = tri->vertex2.x;
      f32 x3 = tri->vertex3.x;

      if (tri->normal.z > 0.0f) {
        if ((y1 - y) * (x2 - x1) - (x1 - x) * (y2 - y1) > 0.0f) continue;
        if ((y2 - y) * (x3 - x2) - (x2 - x) * (y3 - y2) > 0.0f) continue;
        if ((y3 - y) * (x1 - x3) - (x3 - x) * (y1 - y3) > 0.0f) continue;
      }
      else {
        if ((y1 - y) * (x2 - x1) - (x1 - x) * (y2 - y1) < 0.0f) continue;
        if ((y2 - y) * (x3 - x2) - (x2 - x) * (y3 - y2) < 0.0f) continue;
        if ((y3 - y) * (x1 - x3) - (x3 - x) * (y1 - y3) < 0.0f) continue;
      }
    }

    // if (v8035FE10 != 0) {
    //   if (tri->v04 & 0x02)
    //     continue;
    // }
    // else if (tri->type == surface_0072) {
    //   continue;
    // }
    // else if (tri->type == surface_007B && curObj != NULL) {
    //   if ((curObj->v074 & 0x0040) != 0)
    //     continue;

    //   if (curObj == marioObj && (marioState->flags & mf_vanish_cap) != 0)
    //     continue;
    // }

    data->pos.x += tri->normal.x * (radius - offset);
    data->pos.z += tri->normal.z * (radius - offset);
    
    if (data->numSurfaces < 4) {
      data->surfaces[data->numSurfaces] = tri;
      data->numSurfaces += 1;
    }

    numCols += 1;
  }
  
  return numCols;
}


/** 80380E8C(J) */
s32 findWallCols(CollisionData *data) {
  s32 totalCols = 0;
  data->numSurfaces = 0;

  s16 x = (s16) data->pos.x;
  s16 z = (s16) data->pos.z;

  if (x <= -0x2000 || x >= 0x2000) return 0;
  if (z <= -0x2000 || z >= 0x2000) return 0;

  u32 xidx = ((x + 0x2000) / 0x400) & 0xF;
  u32 zidx = ((z + 0x2000) / 0x400) & 0xF;

  SurfaceNode *dynWalls = dynamicPartition[16 * zidx + xidx].walls;
  totalCols += findWallColsFromList(dynWalls, data);

  SurfaceNode *staticWalls = staticPartition[16 * zidx + xidx].walls;
  totalCols += findWallColsFromList(staticWalls, data);
  
  // numFindWallCalls += 1;
  return totalCols;
}


/** 80381038(J) */
Surface *findTriFromListAbove(
  SurfaceNode *triangles,
  s32 x,
  s32 y,
  s32 z,
  f32 *pheight)
{
  while (triangles != NULL) {
    Surface *tri = triangles->head;
    triangles = triangles->tail;

    s32 x1 = tri->vertex1.x;
    s32 z1 = tri->vertex1.z;
    s32 x2 = tri->vertex2.x;
    s32 z2 = tri->vertex2.z;
    s32 x3 = tri->vertex3.x;
    s32 z3 = tri->vertex3.z;

    if ((z1 - z) * (x2 - x1) - (x1 - x) * (z2 - z1) > 0) continue;
    if ((z2 - z) * (x3 - x2) - (x2 - x) * (z3 - z2) > 0) continue;
    if ((z3 - z) * (x1 - x3) - (x3 - x) * (z1 - z3) > 0) continue;

    // if ((v8035FE10 != 0 && !(tri->v04 & 0x02)) ||
    //   (v8035FE10 == 0 && tri->type != surface_0072))
    {
      f32 nx = tri->normal.x;
      f32 ny = tri->normal.y;
      f32 nz = tri->normal.z;
      f32 oo = tri->originOffset;

      if (ny == 0.0f) continue;

      f32 height = -(x * nx + nz * z + oo) / ny;
      if (y - (height - -78.0f) > 0.0f) continue;

      *pheight = height;
      return tri;
    }
  } 

  return NULL;
}


/** 80381264(J) */
f32 findCeil(v3f pos, Surface **pceil) {
  f32 dynHeight = 20000.0f;
  f32 height = 20000.0f;

  s16 x = (s16) pos.x;
  s16 y = (s16) pos.y;
  s16 z = (s16) pos.z;

  *pceil = NULL;

  if (x <= -0x2000 || x >= 0x2000) return height;
  if (z <= -0x2000 || z >= 0x2000) return height;

  u32 xidx = ((x + 0x2000) / 0x400) & 0xF;
  u32 zidx = ((z + 0x2000) / 0x400) & 0xF;

  SurfaceNode *dynCeils = dynamicPartition[16 * zidx + xidx].ceils;
  Surface *dynCeil = findTriFromListAbove(dynCeils, x, y, z, &dynHeight);
  
  SurfaceNode *staticCeils = staticPartition[16 * zidx + xidx].ceils;
  Surface *ceil = findTriFromListAbove(staticCeils, x, y, z, &height);
  
  if (dynHeight < height) {
    ceil = dynCeil;
    height = dynHeight;
  }

  *pceil = ceil;
  return height;
}


/** 8038156C(J) */
Surface *findTriFromListBelow(
  SurfaceNode *triangles,
  s32 x,
  s32 y,
  s32 z,
  f32 *pheight)
{
  while (triangles != NULL) {
    Surface *tri = triangles->head;

    s32 x1 = tri->vertex1.x;
    s32 z1 = tri->vertex1.z;
    s32 x2 = tri->vertex2.x;
    s32 z2 = tri->vertex2.z;
    s32 x3 = tri->vertex3.x;
    s32 z3 = tri->vertex3.z;

    if ((z1 - z) * (x2 - x1) - (x1 - x) * (z2 - z1) < 0) continue;
    if ((z2 - z) * (x3 - x2) - (x2 - x) * (z3 - z2) < 0) continue;
    if ((z3 - z) * (x1 - x3) - (x3 - x) * (z1 - z3) < 0) continue;

    // if ((v8035FE10 != 0 && !(tri->v04 & 0x02)) ||
    //   (v8035FE10 == 0 && tri->type != surface_0072))
    {
      f32 nx = tri->normal.x;
      f32 ny = tri->normal.y;
      f32 nz = tri->normal.z;
      f32 oo = tri->originOffset;

      if (ny == 0.0f) continue;

      f32 height = -(x * nx + nz * z + oo) / ny;
      if (y - (height + -78.0f) < 0.0f) continue;

      *pheight = height;
      return tri;
    }

    triangles = triangles->tail;
  }

  return NULL;
}


/** 80381794(J) */
f32 findFloorHeight(v3f pos) {
  Surface *floor;
  return findFloor(pos, &floor);
}


/** 80381900(J) */
f32 findFloor(v3f pos, Surface **pfloor) {
  f32 dynHeight = -11000.0f;
  f32 height = -11000.0f;

  s16 x = (s16) pos.x;
  s16 y = (s16) pos.y;
  s16 z = (s16) pos.z;
  
  *pfloor = NULL;

  if (x <= -0x2000 || x >= 0x2000) return height;
  if (z <= -0x2000 || z >= 0x2000) return height;

  u32 xidx = ((x + 0x2000) / 0x400) & 0xF;
  u32 zidx = ((z + 0x2000) / 0x400) & 0xF;
  
  SurfaceNode *dynFloors = dynamicPartition[16 * zidx + xidx].floors;
  Surface *dynFloor = findTriFromListBelow(dynFloors, x, y, z, &dynHeight);
  
  SurfaceNode *staticFloors = staticPartition[16 * zidx + xidx].floors;
  Surface *floor = findTriFromListBelow(staticFloors, x, y, z, &height);

  // if (v8035FE12 == 0 && floor != NULL && floor->type == surface_0012)
  //   floor = findTriFromListBelow(
  //     staticFloors, x, (s32) (height - 200.0f), z, &height);
  // else
  //   v8035FE12 = 0;

  // if (floor == NULL)
    // numFindFloorMisses += 1;
  
  if (dynHeight > height) {
    floor = dynFloor;
    height = dynHeight;
  }
  
  *pfloor = floor;
  // numFindFloorCalls += 1;
  return height;
}


bool getFloorHeight(Surface *tri, s16 x, s16 z, f32 *pheight) {
  s32 x1 = tri->vertex1.x;
  s32 z1 = tri->vertex1.z;
  s32 x2 = tri->vertex2.x;
  s32 z2 = tri->vertex2.z;
  s32 x3 = tri->vertex3.x;
  s32 z3 = tri->vertex3.z;

  if ((z1 - z) * (x2 - x1) - (x1 - x) * (z2 - z1) < 0) return false;
  if ((z2 - z) * (x3 - x2) - (x2 - x) * (z3 - z2) < 0) return false;
  if ((z3 - z) * (x1 - x3) - (x3 - x) * (z1 - z3) < 0) return false;

  f32 nx = tri->normal.x;
  f32 ny = tri->normal.y;
  f32 nz = tri->normal.z;
  f32 oo = tri->originOffset;

  if (ny == 0.0f) return false;

  f32 height = -(x * nx + nz * z + oo) / ny;
  // if (y - (height + -78.0f) < 0.0f) return false;

  if (pheight != NULL)
    *pheight = height;
  return true;
}


bool getCeilHeight(Surface *tri, s16 x, s16 z, f32 *pheight) {
  s32 x1 = tri->vertex1.x;
  s32 z1 = tri->vertex1.z;
  s32 x2 = tri->vertex2.x;
  s32 z2 = tri->vertex2.z;
  s32 x3 = tri->vertex3.x;
  s32 z3 = tri->vertex3.z;

  if ((z1 - z) * (x2 - x1) - (x1 - x) * (z2 - z1) > 0) return false;
  if ((z2 - z) * (x3 - x2) - (x2 - x) * (z3 - z2) > 0) return false;
  if ((z3 - z) * (x1 - x3) - (x3 - x) * (z1 - z3) > 0) return false;

  f32 nx = tri->normal.x;
  f32 ny = tri->normal.y;
  f32 nz = tri->normal.z;
  f32 oo = tri->originOffset;

  if (ny == 0.0f) return false;

  f32 height = -(x * nx + nz * z + oo) / ny;
  // if (y - (height - -78.0f) > 0.0f) return false;

  if (pheight != NULL)
    *pheight = height;
  return true;
}
