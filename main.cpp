// main
// -------------------------------------------------------------------
// Copyright (C) 2007 OpenEngine.dk (See AUTHORS)
//
// This program is free software; It is covered by the GNU General
// Public License version 2 or any later version.
// See the GNU General Public License for more details (see LICENSE).
//--------------------------------------------------------------------

// OpenEngine stuff
#include <Meta/Config.h>
#include <Meta/OpenGL.h>
#include <Logging/Logger.h>
#include <Logging/StreamLogger.h>
#include <Core/Engine.h>

#include <Display/Camera.h>
#include <Display/Frustum.h>

#include <Geometry/FaceBuilder.h>
#include <Scene/GeometryNode.h>
#include <Scene/RenderStateNode.h>
#include <Scene/PointLightNode.h>
#include <Scene/TransformationNode.h>

#include <Script/Scheme.h>
#include <Script/ScriptBridge.h>

#include "RayTracer.h"

#include <Renderers/TextureLoader.h>
#include <Display/QtEnvironment.h>
#include <Display/SDLEnvironment.h>
#include <Display/Viewport.h>
#include <Display/HUD.h>
#include <Display/Viewingvolume.h>
#include <Renderers/OpenGL/Renderer.h>
#include <Renderers/OpenGL/RenderingView.h>

#include <Scene/ShapeNode.h>
#include <Scene/SceneNode.h>
#include <Shapes/Sphere.h>
#include <Shapes/Plane.h>

#include <Utils/CameraTool.h>
#include <Utils/ToolChain.h>
#include <Utils/MouseSelection.h>


// Game factory
//#include "GameFactory.h"


#include "EmptyTextureResource.h"
#include "KeyRepeater.h"

// name spaces that we will be using.
// this combined with the above imports is almost the same as
// fx. import OpenEngine.Logging.*; in Java.
using namespace OpenEngine::Logging;
using namespace OpenEngine::Core;
using namespace OpenEngine::Utils;
using namespace OpenEngine::Geometry;
using namespace OpenEngine::Scene;
using namespace OpenEngine::Display;
using namespace OpenEngine::Script;
using namespace OpenEngine::Renderers;
using namespace OpenEngine::Renderers::OpenGL;
using namespace OpenEngine::Shapes;


class QuitHandler : public IListener<KeyboardEventArg> {
    IEngine& engine;
public:
    QuitHandler(IEngine& engine) : engine(engine) {}
    void Handle(KeyboardEventArg arg) {
        if (arg.sym == KEY_ESCAPE) engine.Stop();
    }
};

class RTHandler : public IListener<KeyboardEventArg> {
    RayTracer& rt;
public:
    RTHandler(RayTracer& rt) : rt(rt) {}
    void Handle(KeyboardEventArg arg) {
        if (arg.type == EVENT_RELEASE)
            return;
        if (arg.sym == KEY_LEFT) {
            rt.markX--;
        }
        else if (arg.sym == KEY_RIGHT) {
            rt.markX++;
        }
        else if (arg.sym == KEY_UP) {
            rt.markY++;
        }
        else if (arg.sym == KEY_DOWN) {
            rt.markY--;
        }
        else if (arg.sym == KEY_p) {
            rt.markDebug = true;
            //rt.GetRayTracerDebugNode()->markDebug = true;
        }

    }
};


class RTRenderingView : public RenderingView {
public:
    RTRenderingView(Viewport& viewport) : IRenderingView(viewport),RenderingView(viewport) {}

    void VisitShapeNode(ShapeNode* node) {
        IRenderer *renderer = GetRenderer();

        Shape *shape = node->shape;

        Shapes::Sphere *sphere = dynamic_cast<Shapes::Sphere*>(shape);
        Shapes::Plane *plane = dynamic_cast<Shapes::Plane*>(shape);

        if (sphere) {

            glEnable(GL_LIGHTING);
            glEnable(GL_COLOR_MATERIAL);

            RenderingView::ApplyMaterial(sphere->mat);

            Vector<3,float> center = sphere->center;
            float radius = sphere->radius;
            Vector<4,float> color = Vector<4,float>(1,0,0,1);

            //logger.info << "  hesten" << logger.end;



            CHECK_FOR_GL_ERROR();

            glPushMatrix();
            glTranslatef(center[0], center[1], center[2]);
            glColor3f(color[0], color[1], color[2]);
            GLUquadricObj* qobj = gluNewQuadric();
            glLineWidth(1);
            gluQuadricNormals(qobj, GLU_SMOOTH);
            gluQuadricDrawStyle(qobj, GLU_LINE);
            //gluQuadricDrawStyle(qobj, GLU_FILL);
            gluQuadricOrientation(qobj, GLU_INSIDE);
            gluSphere(qobj, radius, 15, 15);
            gluDeleteQuadric(qobj);
            glPopMatrix();

            // reset state

            CHECK_FOR_GL_ERROR();

        }
        else if (plane) {
                // draw a thicker line in origin
            float dist = 0.0;
            float space = 10.0;
            unsigned int number = 100;
            float width = space*number;
            Vector<3,float> solidColor;
            bool solidRepeat = true;
            Vector<3,float> fadeColor = solidColor;

            renderer->DrawLine( Line(Vector<3,float>(0.0, 0.0, -width),
                                                Vector<3,float>(0.0, 0.0,  width)), solidColor, 2);
            renderer->DrawLine( Line(Vector<3,float>(-width, 0.0, 0.0),
                                                Vector<3,float>( width, 0.0, 0.0)), solidColor, 2);

            // draw (number-1) lines in all directions going outwards from origin
            for (unsigned int i = 1; i < number; i++) {
                dist += space;
                Vector<3,float> c = (i % solidRepeat == 0) ? solidColor : fadeColor;
                renderer->DrawLine( Line(Vector<3,float>(  dist, 0.0, -width),
                                                    Vector<3,float>(  dist, 0.0,  width)), c);
                renderer->DrawLine( Line(Vector<3,float>( -dist, 0.0, -width),
                                                    Vector<3,float>( -dist, 0.0,  width)), c);
                renderer->DrawLine( Line(Vector<3,float>(-width, 0.0,   dist),
                                                    Vector<3,float>( width, 0.0,   dist)), c);
                renderer->DrawLine( Line(Vector<3,float>(-width, 0.0,  -dist),
                                                    Vector<3,float>( width, 0.0,  -dist)), c);
            }

        }
    }
};

struct Config {
    IEngine&                engine;
    IFrame*                 frame;
    Viewport*               viewport;
    IViewingVolume*         viewingvolume;
    Camera*                 camera;
    Frustum*                frustum;
    IRenderer*              renderer;
    IMouse*                 mouse;
    IKeyboard*              keyboard;
    ISceneNode*             scene;
    MouseSelection*         ms;
    RayTracer*              rt;
    HUD*                    hud;
    EmptyTextureResourcePtr traceTex;



    OpenEngine::Renderers::TextureLoader* textureLoader;

    Config(IEngine& engine)
        : engine(engine)
        , frame(NULL)
        , viewport(NULL)
        , viewingvolume(NULL)
        , camera(NULL)
        , frustum(NULL)
        , renderer(NULL)
        , mouse(NULL)
        , keyboard(NULL)
        , scene(NULL)
        , ms(NULL)
        , rt(NULL)
        , hud(NULL)
    //, traceTex(0)
        , textureLoader(NULL)
    {}
};

// Forward declaration of the setup methods
void SetupResources(Config&);
void SetupDevices(Config&);
void SetupDisplay(Config&);
void SetupRendering(Config&);
void SetupScene(Config&);
void SetupDebugging(Config&);
void SetupRayTracer(Config&);


int main(int argc, char** argv) {
    // Setup logging facilities.
    Logger::AddLogger(new StreamLogger(&std::cout));

    // Print usage info.
    logger.info << "========= Running OpenEngine Test Project =========" << logger.end;




    Engine *engine = new Engine();
    Config config(*engine);

    // Setup the engine
    SetupResources(config);
    SetupDisplay(config);
    SetupDevices(config);
    SetupRendering(config);
    SetupScene(config);
    SetupRayTracer(config);

    // Possibly add some debugging stuff
    SetupDebugging(config);

    config.rt->Start();

    // Start up the engine.
    engine->Start();

    config.rt->run = false;
    config.rt->Wait();

    // release event system
    // post condition: scene and modules are not processed
    delete engine;

    delete config.scene;

    // Return when the engine stops.
    return EXIT_SUCCESS;
}
/*


    // Create simple setup
    QtEnvironment* env = new QtEnvironment(true,1024,768);
    Viewport* vp = new Viewport(env->GetFrame());
    RenderingView *rv = new RTRenderingView(*vp);

    SimpleSetup* setup = new SimpleSetup("Ray Tracer",
                                         vp,env,rv);
    setup->AddDataDirectory("projects/RayTracer/data/");
    RenderStateNode* rn = new RenderStateNode();
    rn->EnableOption(RenderStateNode::COLOR_MATERIAL);
    rn->EnableOption(RenderStateNode::LIGHTING);
    rn->EnableOption(RenderStateNode::BACKFACE);
    //rn->EnableOption(RenderStateNode::WIREFRAME);

    ISceneNode* root = setup->GetScene();
    root->AddNode(rn);
    root = rn;

    Scheme *s = new Scheme();
    s->AddFileToAutoLoad("init.scm");
    s->AddFileToAutoLoad("oe-init.scm");
    s->AddFileToAutoLoad("test.scm");
    //s->EvalAndPrint("(display (+ 1 2))");
    setup->GetEngine().ProcessEvent().Attach(*s);




    FaceSet *fs = new FaceSet();
    FaceBuilder::FaceState state;
    state.color = Vector<4,float>(1,0,0,1);

    FaceBuilder::MakeABox(fs, state, Vector<3,float>(0,0,0), Vector<3,float>(10,10,10));
    GeometryNode *gn = new GeometryNode(fs);
    //root->AddNode(gn);


    FaceSet *fs1 = new FaceSet();
    state.color = Vector<4,float>(0,1,0,1);

    FaceBuilder::MakeASphere(fs1, state, Vector<3,float>(-20,0,0), 20, 10);
    GeometryNode *gn2 = new GeometryNode(fs1);
    TransformationNode *sphereTrans = new TransformationNode();
    sphereTrans->SetPosition(Vector<3,float>(10,10,10));
    sphereTrans->AddNode(gn2);
    //root->AddNode(sphereTrans);

    ScriptBridge::AddHandler<TransformationNode>(new TransformationNodeHandler());
    ScriptBridge::AddHandler<Vector<3,float> >(new VectorHandler());

    sbo sb = ScriptBridge::CreateSboPointer<TransformationNode>(sphereTrans);


    s->DefineSbo("dot-tn",sb);

    PointLightNode *pln = new PointLightNode();
    TransformationNode *lightTn = new TransformationNode();

    lightTn->SetPosition(Vector<3,float>(0,100,0));

    lightTn->AddNode(pln);
    root->AddNode(lightTn);



    Camera *cam = setup->GetCamera();
    cam->SetPosition(Vector<3,float>(0,0,0));
    cam->LookAt(Vector<3,float>(0,0,-1));

    EmptyTextureResourcePtr traceTexture = EmptyTextureResource::Create(400,300,24);
    traceTexture->Load();
    setup->GetTextureLoader().Load(traceTexture, TextureLoader::RELOAD_QUEUED);



    ShapeNode *sn1 = new ShapeNode(new Shapes::Sphere(Vector<3,float>(10,10,-100), 15));
    root->AddNode(sn1);
    TransformationNode *tn2 = new TransformationNode();
    tn2->Move(0,-10,0);
    ShapeNode *sn2 = new ShapeNode(new Shapes::Plane(Vector<3,float>(0,-10,0),
                                                     Vector<3,float>(0,-10,1),
                                                     Vector<3,float>(1,-10,0)));
    tn2->AddNode(sn2);
    root->AddNode(tn2);

    RayTracer *rt = new RayTracer(traceTexture,root);
    //TransformationNode* traceTexNode = CreateTextureBillboard(traceTexture,1.0);
    //traceTexNode->Rotate(0,180,0);
    //root->AddNode(traceTexNode);

    root->AddNode(rt->GetRayTracerDebugNode());

    // add ray trace hud.
    // RayTracerPtr rt = RayTracer::Create();
    // setup->GetTextureLoader().Load(rt, TextureLoader::RELOAD_QUEUED);
    HUD::Surface *rtHud = setup->GetHUD().CreateSurface(traceTexture);
    rtHud->SetPosition(HUD::Surface::LEFT,
                        HUD::Surface::TOP);

    setup->GetEngine().ProcessEvent().Attach(*rt);

    rt->Start();
    // Start the engine.
    setup->GetEngine().Start();

    rt->run = false;
    rt->Wait();

    // Return when the engine stops.
    return EXIT_SUCCESS;
}
*/

void SetupResources(Config& config) {
    // set the resources directory
    // @todo we should check that this path exists
    // set the resources directory
    //string resources = "projects/Selection/data/";
    //DirectoryManager::AppendPath(resources);

    // load resource plug-ins
    //ResourceManager<IModelResource>::AddPlugin(new OBJPlugin());
    //ResourceManager<ITextureResource>::AddPlugin(new SDLImagePlugin());
    //ResourceManager<ITextureResource>::AddPlugin(new TGAPlugin());
}

void SetupDisplay(Config& config) {
    if (config.frame         != NULL ||
        config.viewingvolume != NULL ||
        config.camera        != NULL ||
        config.frustum       != NULL ||
        config.viewport      != NULL)
        throw Exception("Setup display dependencies are not satisfied.");

    config.frame         = new SDLFrame(800, 600, 32);
    config.viewingvolume = new ViewingVolume();
    config.camera        = new Camera( *config.viewingvolume );
    config.camera->SetPosition(Vector<3,float>(0,0,0));
    config.camera->LookAt(Vector<3,float>(0,0,-1));
    //config.frustum       = new Frustum(*config.camera, 20, 3000);
    config.viewport      = new Viewport(0,0,399,299);
    config.viewport->SetViewingVolume(config.camera);

    config.engine.InitializeEvent().Attach(*config.frame);
    config.engine.ProcessEvent().Attach(*config.frame);
    config.engine.DeinitializeEvent().Attach(*config.frame);
}

void SetupDevices(Config& config) {
    if (config.keyboard != NULL ||
        config.mouse    != NULL)
        throw Exception("Setup devices dependencies are not satisfied.");
    // Create the mouse and keyboard input modules
    SDLInput* input = new SDLInput();
    config.keyboard = input;
    config.mouse = input;

    // Bind the quit handler
    QuitHandler* quit_h = new QuitHandler(config.engine);
    config.keyboard->KeyEvent().Attach(*quit_h);

    // Bind to the engine for processing time
    config.engine.InitializeEvent().Attach(*input);
    config.engine.ProcessEvent().Attach(*input);
    config.engine.DeinitializeEvent().Attach(*input);
}

void SetupRendering(Config& config) {
    if (config.viewport == NULL ||
        config.renderer != NULL ||
        config.camera == NULL)
        throw Exception("Setup renderer dependencies are not satisfied.");

    // Create a renderer
    config.renderer = new OpenGL::Renderer(config.viewport);
    //config.renderer = new BufferedRenderer(config.viewport);

    // Setup a rendering view
    IRenderingView* rv = new RTRenderingView(*config.viewport);
    config.renderer->ProcessEvent().Attach(*rv);

    // Add rendering initialization tasks
    config.textureLoader = new OpenEngine::Renderers::TextureLoader(*config.renderer);
    config.renderer->PreProcessEvent().Attach(*config.textureLoader);

    //    DisplayListTransformer* dlt = new DisplayListTransformer(rv);
    //    config.renderer->InitializeEvent().Attach(*dlt);

    //    config.renderer->PreProcessEvent()
    //    .Attach( *(new OpenEngine::Renderers::OpenGL::LightRenderer(*config.camera)) );

    config.engine.InitializeEvent().Attach(*config.renderer);
    config.engine.ProcessEvent().Attach(*config.renderer);
    config.engine.DeinitializeEvent().Attach(*config.renderer);


    config.hud = new HUD();
    config.renderer->PostProcessEvent().Attach(*config.hud);

    // mouse selector stuff
    //SelectionSet<ISceneNode>* ss = new SelectionSet<ISceneNode>();
    config.ms = new MouseSelection(*config.frame, *config.mouse, NULL);

    //TransformationTool* tt = new TransformationTool(*config.textureLoader);
    //ss->ChangedEvent().Attach(*tt);
    CameraTool* ct   = new CameraTool();
    ToolChain* tc    = new ToolChain();
    //SelectionTool* st = new SelectionTool(*ss);
    tc->PushBackTool(ct);
    //tc->PushBackTool(tt);
    //tc->PushBackTool(st);

    config.renderer->PostProcessEvent().Attach(*config.ms);
    config.mouse->MouseMovedEvent().Attach(*config.ms);
    config.mouse->MouseButtonEvent().Attach(*config.ms);
    config.keyboard->KeyEvent().Attach(*config.ms);

    //add frustrum cameras
    int width = 800;
    int height = 600;
    float dist = 100;

    // bottom right
    Camera* cam_br = new Camera(*(new ViewingVolume()));
    cam_br->SetPosition(Vector<3,float>(20,0,20));
    cam_br->LookAt(0,0,-80);
    Viewport* vp_br = new Viewport(width/2, 0, width,height/2);
    vp_br->SetViewingVolume(cam_br);
    OpenGL::RenderingView* rv_br = new RTRenderingView(*vp_br);
    config.renderer->ProcessEvent().Attach(*rv_br);
    // top right
    Camera* cam_tr = new Camera(*(new ViewingVolume()));
    cam_tr->SetPosition(Vector<3,float>(0,150,-100));
    cam_tr->LookAt(0,0,-100);
    Viewport* vp_tr = new Viewport(width/2,height/2, width,height);
    vp_tr->SetViewingVolume(cam_tr);
    OpenGL::RenderingView* rv_tr = new RTRenderingView(*vp_tr);
    config.renderer->ProcessEvent().Attach(*rv_tr);

    // top left
    Camera* cam_tl = new Camera(*(new ViewingVolume()));
    cam_tl->SetPosition(Vector<3,float>(dist,0,0));
    cam_tl->LookAt(0,0,0);
    Viewport* vp_tl = new Viewport(0,height/2, width/2,height);
        //new Viewport(800,600);
    vp_tl->SetViewingVolume(cam_tl);
    OpenGL::RenderingView* rv_tl = new RTRenderingView(*vp_tl);
    config.renderer->ProcessEvent().Attach(*rv_tl);

    config.ms->BindTool(config.viewport, tc);
    config.ms->BindTool(vp_br, tc);
    config.ms->BindTool(vp_tl, tc);
    config.ms->BindTool(vp_tr, tc);

}

void SetupScene(Config& config) {
    if (config.scene  != NULL ||
        config.mouse  == NULL ||
        config.keyboard == NULL)
        throw Exception("Setup scene dependencies are not satisfied.");

    // Create a root scene node


    RenderStateNode* rn = new RenderStateNode();
    rn->EnableOption(RenderStateNode::COLOR_MATERIAL);
    rn->EnableOption(RenderStateNode::LIGHTING);
    //rn->EnableOption(RenderStateNode::BACKFACE);

    ISceneNode* root = config.scene = rn;

    PointLightNode *pln = new PointLightNode();
    pln->diffuse = Vector<4,float>(.5,.5,.5,1);
    TransformationNode *lightTn = new TransformationNode();

    lightTn->SetPosition(Vector<3,float>(0,10,0));

    lightTn->AddNode(pln);
    root->AddNode(lightTn);


    // Shapes

    ShapeNode *sn1 = new ShapeNode(new Shapes::Sphere(Vector<3,float>(10,10,-100), 15));
    root->AddNode(sn1);
    TransformationNode *tn2 = new TransformationNode();
    tn2->Move(0,-10,0);
    ShapeNode *sn2 = new ShapeNode(new Shapes::Plane(Vector<3,float>(0,-10,0),
                                                     Vector<3,float>(0,-10,1),
                                                     Vector<3,float>(1,-10,0)));
    tn2->AddNode(sn2);
    root->AddNode(tn2);




    config.traceTex = EmptyTextureResource::Create(400,300,24);
    config.traceTex->Load();
    config.textureLoader->Load(config.traceTex, TextureLoader::RELOAD_QUEUED);

    config.ms->SetScene(config.scene);
    config.textureLoader->Load(*config.scene);

    HUD::Surface *rtHud = config.hud->CreateSurface(config.traceTex);
    rtHud->SetPosition(HUD::Surface::LEFT,
                       HUD::Surface::TOP);
    rtHud->SetScale(2.0,2.0);



    // Supply the scene to the renderer
    config.renderer->SetSceneRoot(config.scene);

    //config.textureLoader->SetDefaultReloadPolicy(OpenEngine::Renderers::TextureLoader::RELOAD_NEVER);
}

void SetupDebugging(Config& config) {



    config.scene->AddNode(config.rt->GetRayTracerDebugNode());

}


void SetupRayTracer(Config& config) {
    config.rt = new RayTracer(config.traceTex, config.camera, config.scene);

    config.engine.ProcessEvent().Attach(*config.rt);
    
    KeyRepeater* krp = new KeyRepeater();

    config.engine.ProcessEvent().Attach(*krp);
    config.keyboard->KeyEvent().Attach(*krp);

    RTHandler* rt_h = new RTHandler(*config.rt);
    krp->KeyEvent().Attach(*rt_h);

}
