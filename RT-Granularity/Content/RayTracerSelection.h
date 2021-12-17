#pragma once

#define PER_PIXEL_RT      1
#define PER_VERTEX_RT     2
#define PER_TES_VERTEX_RT 3

#define RAYTRACER_TYPE PER_PIXEL_RT

#if RAYTRACER_TYPE == PER_PIXEL_RT
# include "PRayTracer.h"
using RayTracer = PRayTracer;
#elif RAYTRACER_TYPE == PER_VERTEX_RT
# include "VRayTracer.h"
using RayTracer = VRayTracer;
#elif RAYTRACER_TYPE == PER_TES_VERTEX_RT
# include "TVRayTracer.h"
using RayTracer = TVRayTracer;
#else
# error "RAYTRACER_TYPE is not properly defined"
#endif
