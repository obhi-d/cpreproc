
#define DEFINE2(x, y, z) x##y
#define DEFINE3          2
#define DEFINE4          DEFINE2(1,1,2) * 6

#define DEFINE_A

#define DEFINE_B 4

struct Camera
{
  mat4 viewMatrix[DEFINE4];
  mat4 projMatrix;
};

#define first 0
#define second 1
#define fourth 2

#pragma OK

#pragma OK 

#pragma OK - branch

