#ifndef _RT_RAY_TRACER_H_
#define _RT_RAY_TRACER_H_

#include <boost/serialization/weak_ptr.hpp>
#include <Resources/ITextureResource.h>


class RayTracer;
typedef boost::shared_ptr<RayTracer> RayTracerPtr;

class RayTracer : public OpenEngine::Resources::ITextureResource {
public:
    static RayTracerPtr Create();

};

#endif
