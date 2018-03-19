#ifndef UTIL_H
#define UTIL_H


#include <stdint.h>


typedef int8_t s8; 
typedef int16_t s16;
typedef int32_t s32;

typedef uint8_t u8; 
typedef uint16_t u16;
typedef uint32_t u32;

typedef float f32;
typedef double f64;


typedef u8 bool;
#define true 1
#define false 0


typedef f32 Mtxf[4][4];
typedef f32 (*Mtxfp)[4];


typedef struct {
  f32 x;
  f32 y;
  f32 z;
} v3f;


typedef struct {
  union { s32 x; s32 pitch; };
  union { s32 y; s32 yaw;   };
  union { s32 z; s32 roll;  };
} v3i;


typedef struct {
  union { s16 x; s16 pitch; };
  union { s16 y; s16 yaw;   };
  union { s16 z; s16 roll;  };
} v3h;


extern u16 rngState;


s16 atan2xy(f32 x, f32 y);
void matrixFromTransAndRot(Mtxfp dst, v3f *translate, v3h *rotate);
void matrixVecMult(Mtxfp m, v3f *dst, v3f *src);
void matrixTransposeVecMult(Mtxfp m, v3f *dst, v3f *src);
f32 incTowardAsymF(f32 speed, f32 target, f32 posDelta, f32 negDelta);
bool incTowardSymFP(f32 *x, f32 target, f32 delta);
u16 randomU16(void);
s32 randomUnit();


extern u32 sineTableRaw[0x1400];

#define sinTable ((f32 *) &sineTableRaw[0])
#define cosTable ((f32 *) &sineTableRaw[1024])
#define sins(x) sinTable[(u16) (x) >> 4]
#define coss(x) cosTable[(u16) (x) >> 4]


#endif