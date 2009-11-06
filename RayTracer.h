#ifndef _RT_RAY_TRACER_H_
#define _RT_RAY_TRACER_H_

#include <boost/serialization/weak_ptr.hpp>
#include <Resources/ITextureResource.h>


class RayTracer;
typedef boost::shared_ptr<RayTracer> RayTracerPtr;

using namespace OpenEngine;

class RayTracer : public Resources::ITextureResource {

    int _id;
public:
    static RayTracerPtr Create();

    void Load();
    void Unload();
    int GetID() {return _id;}
    void SetID(int id) {_id = id;}
    unsigned int GetWidth() {return 100;}
    unsigned int GetHeight() {return 100;}
    unsigned int GetDepth() {return 4;}
    unsigned char* GetData();
    Resources::ColorFormat GetColorFormat() {}

};

#endif
