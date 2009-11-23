#ifndef _RT_RAY_TRACER_H_
#define _RT_RAY_TRACER_H_

#include <Logging/Logger.h>
#include <Core/Thread.h>
#include <Core/EngineEvents.h>

#include <Math/Vector.h>
#include <vector>
#include <Shapes/Shape.h>
#include <Scene/ISceneNodeVisitor.h>
#include <Utils/Timer.h>

#include "EmptyTextureResource.h"

using namespace OpenEngine;
using namespace OpenEngine::Resources;
using namespace OpenEngine::Core;
using namespace OpenEngine::Math;
using namespace OpenEngine::Utils;
using namespace OpenEngine::Shapes;
using namespace OpenEngine::Scene;
using namespace std;


class RayTracer : public Thread , public IListener<ProcessEventArg>, public ISceneNodeVisitor {

    
    struct Light {
        Vector<3,float> pos;
        Vector<4,float> color;
    };

    struct Object {
        Shape *shape;
    };

    vector<Light> lights;
    vector<Object> objects;

    Vector<3,float> camPos;
    float fovX;
    float fovY;

    float height,width;

    int maxDepth;

    EmptyTextureResourcePtr texture;
    
    int traceNum;
    bool dirty;

    Vector<4,float> TraceRay(Ray r, int depth);
    void Trace();
    Timer timer;
    ISceneNode* root;

    

public:
    bool run;

    
    RayTracer(EmptyTextureResourcePtr tex, ISceneNode* root);
    void Run();

    void Handle(ProcessEventArg arg);

    void VisitShapeNode(ShapeNode* node);
};

#endif
