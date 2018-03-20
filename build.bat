
gcc ^
  -DWIN32 ^
  -mwindows ^
  -mconsole ^
  -std=c99 ^
  -O3 ^
  -Wall -Wextra ^
  -Wno-missing-braces ^
  -Wno-incompatible-pointer-types ^
  source/*.c ^
  -LC:\Dev\GLFW\lib ^
  -lglfw3 ^
  -lopengl32 ^
  -fwrapv ^
  -fno-strict-aliasing ^
  -IC:\Dev\GLFW\include ^
  -o ship.exe
