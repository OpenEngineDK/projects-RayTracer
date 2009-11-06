#include "RayTracer.h"

RayTracerPtr RayTracer::Create() {
    return RayTracerPtr(new RayTracer());
}
