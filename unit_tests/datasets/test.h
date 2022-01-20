
#define DEFINE2(x, y, z) x##y
#define DEFINE3          2
#define DEFINE4          DEFINE2(1,1,2) * 6

#if DEFINE2(4, 4, 4) < 46
#define DEFINE_A
#endif

#ifdef DEFINE_A
#define DEFINE_B 4
#endif

struct Camera
{
  mat4 viewMatrix[DEFINE4];
  mat4 projMatrix;
};

#define first 0
#define second 1
#define fourth 2

#if !defined first
#error unexpected first
#elif !defined second
#error unexpected second
#elif defined third
#error unexpected third
#else 
#pragma OK
#endif

#if !defined first
#error unexpected first
#elif !defined second
#error unexpected second
#elif defined fourth
#pragma OK 
#else
#error unexpected third
#endif

#if defined first
#if defined second
#if defined third
#error unexpected
#else
#pragma OK - branch
#endif
#endif
#endif
