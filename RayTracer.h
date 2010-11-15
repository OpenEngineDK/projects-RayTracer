#ifndef _RT_RAY_TRACER_H_
#define _RT_RAY_TRACER_H_

#include <Logging/Logger.h>
#include <Core/Thread.h>
#include <Core/Mutex.h>
#include <Core/EngineEvents.h>

#include <Math/Vector.h>
#include <vector>
#include <Shapes/Shape.h>
#include <Scene/ISceneNodeVisitor.h>
#include <Scene/RenderNode.h>
#include <Utils/Timer.h>
#include <Renderers/IRenderingView.h>
#include <Display/IViewingVolume.h>

#include <Resources/EmptyTextureResource.h>

using namespace OpenEngine;
using namespace OpenEngine::Resources;
using namespace OpenEngine::Renderers;
using namespace OpenEngine::Core;
using namespace OpenEngine::Math;
using namespace OpenEngine::Utils;
using namespace OpenEngine::Shapes;
using namespace OpenEngine::Scene;
using namespace OpenEngine::Display;

using namespace std;



class RayTracer : public Thread 
                , public IListener<Core::ProcessEventArg>
                , public ISceneNodeVisitor {

    struct RayHit {
        Ray r;
        Vector<3,float> p;
        //float t;
    };

    class RayTracerRenderNode : public RenderNode {
        RayTracer* rt;
    public:
        bool markDebug;

        RayTracerRenderNode(RayTracer* rt) : rt(rt) {}

        virtual void Apply(RenderingEventArg arg, ISceneNodeVisitor& v);
    };

    RayTracerRenderNode *rnode;

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

    Shape* NearestShape(Ray r, 
                        Vector<3,float>& p, 
                        bool debug= false,
                        Hit side=HIT_OUT
                        );

    Ray RayForPoint(unsigned int u, unsigned int v);

    Vector<4,float> TraceRay(const Ray r, 
                             int depth, 
                             bool debug=false, 
                             Hit side = HIT_OUT,
                             float rIndex=1.0, 
                             list<RayHit>* rayCollection=NULL);
    void Trace();
    Timer timer;
    ISceneNode* root;

    Mutex objectsLock;

    IViewingVolume* volume;

    Matrix<4,4,float> _tmpIV;
    Matrix<4,4,float> _tmpProj;

    Vector<3,float> _tmpOrigo;
    

public:
    bool run;

    unsigned int markX;
    unsigned int markY;
    bool markDebug;

    
    RayTracer(EmptyTextureResourcePtr tex, IViewingVolume* vol, ISceneNode* root);
    void Run();

    void Handle(Core::ProcessEventArg arg);

    void VisitShapeNode(ShapeNode* node);

    RayTracerRenderNode* GetRayTracerDebugNode();
};

#endif
