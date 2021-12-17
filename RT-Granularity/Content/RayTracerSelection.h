#pragma once

#define PER_PIXEL      1
#define PER_VERTEX     2
#define PER_TES_VERTEX 3

#define RAYTRACER_TYPE PER_PIXEL

#if RAYTRACER_TYPE == PER_PIXEL

# include "PRayTracer.h"
using RayTracer = PRayTracer;
#define RayTracerTypeName L"Per-pixel Ray Tracing"

#elif RAYTRACER_TYPE == PER_VERTEX

# include "VRayTracer.h"
using RayTracer = VRayTracer;
#define RayTracerTypeName L"Per-vertex Ray Tracing"

#elif RAYTRACER_TYPE == PER_TES_VERTEX

# include "TVRayTracer.h"
using RayTracer = TVRayTracer;
#define RayTracerTypeName L"Per-tessellated-vertex Ray Tracing"

#else
# error "RAYTRACER_TYPE is not properly defined"
#endif

#undef PER_PIXEL
#undef PER_VERTEX
#undef PER_TES_VERTEX
#undef RAYTRACER_TYPE
