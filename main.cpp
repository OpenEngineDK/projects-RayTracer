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
//#include <Meta/OpenCL.h>
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


#include <Display/QtEnvironment.h>
#include <Display/SDLEnvironment.h>
#include <Display/Viewport.h>
#include <Display/HUD.h>
#include <Display/ViewingVolume.h>

#include <Renderers/OpenGL/Renderer.h>
#include <Renderers/OpenGL/RenderingView.h>
#include <Display/OpenGL/TextureCopy.h>
#include <Display/RenderCanvas.h>
#include <Renderers/TextureLoader.h>

#include <Resources/File.h>

#include <Scene/ShapeNode.h>
#include <Scene/SceneNode.h>
#include <Shapes/Sphere.h>
#include <Shapes/Ellipsoid.h>
#include <Shapes/Plane.h>

#include <Utils/CameraTool.h>
#include <Utils/ToolChain.h>
#include <Utils/MouseSelection.h>
#include <Utils/BetterMoveHandler.h>

// Game factory
//#include "GameFactory.h"


#include <Resources/EmptyTextureResource.h>
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
using namespace OpenEngine::Display::OpenGL;
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
            rt.GetRayTracerDebugNode()->markDebug = true;
        }

    }
};


class RTRenderingView : public RenderingView {
public:
    RTRenderingView() : RenderingView() {}

    void VisitShapeNode(ShapeNode* node) {
        IRenderer& renderer = arg->renderer;

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

            renderer.DrawLine( Line(Vector<3,float>(0.0, 0.0, -width),
                                                Vector<3,float>(0.0, 0.0,  width)), solidColor, 2);
            renderer.DrawLine( Line(Vector<3,float>(-width, 0.0, 0.0),
                                                Vector<3,float>( width, 0.0, 0.0)), solidColor, 2);

            // draw (number-1) lines in all directions going outwards from origin
            for (unsigned int i = 1; i < number; i++) {
                dist += space;
                Vector<3,float> c = (i % solidRepeat == 0) ? solidColor : fadeColor;
                renderer.DrawLine( Line(Vector<3,float>(  dist, 0.0, -width),
                                                    Vector<3,float>(  dist, 0.0,  width)), c);
                renderer.DrawLine( Line(Vector<3,float>( -dist, 0.0, -width),
                                                    Vector<3,float>( -dist, 0.0,  width)), c);
                renderer.DrawLine( Line(Vector<3,float>(-width, 0.0,   dist),
                                                    Vector<3,float>( width, 0.0,   dist)), c);
                renderer.DrawLine( Line(Vector<3,float>(-width, 0.0,  -dist),
                                                    Vector<3,float>( width, 0.0,  -dist)), c);
            }

        }
    }
};

struct Config {
    IEngine&                engine;
    IFrame*                 frame;
    IRenderCanvas*          canvas;
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
        , canvas(NULL)
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
void SetupOpenCL(Config&);
void SetupOpenCLhpp(Config&);

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
    
    //SetupOpenCL(config);
    //SetupOpenCLhpp(config);

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
        config.canvas        != NULL)
        throw Exception("Setup display dependencies are not satisfied.");

    config.frame         = new SDLFrame(800, 600, 32);
    config.viewingvolume = new ViewingVolume();
    config.camera        = new Camera( *config.viewingvolume );
    config.camera->SetPosition(Vector<3,float>(30,20,-80));
    config.camera->LookAt(Vector<3,float>(-20,10,-100));
    //config.frustum       = new Frustum(*config.camera, 20, 3000);
    config.canvas = new RenderCanvas(new TextureCopy());
    config.canvas->SetViewingVolume(config.camera);
    //config.viewport      = new Viewport(0,0,399,299);
    //config.viewport->SetViewingVolume(config.camera);

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
    
    // movehandler
    BetterMoveHandler* mov = new BetterMoveHandler(*config.camera, *config.mouse);
    config.keyboard->KeyEvent().Attach(*mov);
    config.mouse->MouseMovedEvent().Attach(*mov);
    config.mouse->MouseButtonEvent().Attach(*mov);

    config.engine.ProcessEvent().Attach(*mov);

    // Bind to the engine for processing time
    config.engine.InitializeEvent().Attach(*input);
    config.engine.ProcessEvent().Attach(*input);
    config.engine.DeinitializeEvent().Attach(*input);
}

void SetupRendering(Config& config) {
    if (config.canvas == NULL ||
        config.renderer != NULL ||
        config.camera == NULL)
        throw Exception("Setup renderer dependencies are not satisfied.");

    // Create a renderer
    config.renderer = new Renderers::OpenGL::Renderer();
    //config.renderer = new BufferedRenderer(config.viewport);

    // Setup a rendering view
    IRenderingView* rv = new RTRenderingView();
    config.renderer->ProcessEvent().Attach(*rv);
    config.canvas->SetRenderer(config.renderer);
    config.frame->SetCanvas(config.canvas);

    // Add rendering initialization tasks
    config.textureLoader = new OpenEngine::Renderers::TextureLoader(*config.renderer);
    config.renderer->PreProcessEvent().Attach(*config.textureLoader);

    //    DisplayListTransformer* dlt = new DisplayListTransformer(rv);
    //    config.renderer->InitializeEvent().Attach(*dlt);

    //    config.renderer->PreProcessEvent()
    //    .Attach( *(new OpenEngine::Renderers::OpenGL::LightRenderer(*config.camera)) );



    config.hud = new HUD();
    config.renderer->PostProcessEvent().Attach(*config.hud);

    // mouse selector stuff
    //SelectionSet<ISceneNode>* ss = new SelectionSet<ISceneNode>();
    //config.ms = new MouseSelection(*config.frame, *config.mouse, NULL);

    //TransformationTool* tt = new TransformationTool(*config.textureLoader);
    //ss->ChangedEvent().Attach(*tt);
    // CameraTool* ct   = new CameraTool();
    // ToolChain* tc    = new ToolChain();
    // tc->PushBackTool(ct);

    // CameraTool* ct2   = new CameraTool();
    // ToolChain* tc2    = new ToolChain();
    // tc2->PushBackTool(ct2);

    // CameraTool* ct3   = new CameraTool();
    // ToolChain* tc3    = new ToolChain();
    // tc3->PushBackTool(ct3);

    // config.renderer->PostProcessEvent().Attach(*config.ms);
    // config.mouse->MouseMovedEvent().Attach(*config.ms);
    // config.mouse->MouseButtonEvent().Attach(*config.ms);
    // config.keyboard->KeyEvent().Attach(*config.ms);

    //add frustrum cameras
    int width = 800;
    int height = 600;
    float dist = 100;

    // rv = new RTRenderingView();
    // Camera *cam = new Camera(*(new Viewingvolume()));
    // rv->SetViewingVolume(cam);
    // cam->SetPosition(Vector<3,float>(20,0,20));
    // cam->LookAt(0,0,-80);
    

    // // bottom right
    // Camera* cam_br = new Camera(*(new ViewingVolume()));
    // cam_br->SetPosition(Vector<3,float>(20,0,20));
    // cam_br->LookAt(0,0,-80);
    // Viewport* vp_br = new Viewport(width/2, 0, width,height/2);
    // vp_br->SetViewingVolume(cam_br);
    // Renderer::OpenGL::RenderingView* rv_br = new RTRenderingView(*vp_br);
    // config.renderer->ProcessEvent().Attach(*rv_br);
    // // top right
    // Camera* cam_tr = new Camera(*(new ViewingVolume()));
    // cam_tr->SetPosition(Vector<3,float>(0,150,-100));
    // cam_tr->LookAt(0,0,-100);
    // Viewport* vp_tr = new Viewport(width/2,height/2, width,height);
    // vp_tr->SetViewingVolume(cam_tr);
    // Renderer::OpenGL::RenderingView* rv_tr = new RTRenderingView(*vp_tr);
    // config.renderer->ProcessEvent().Attach(*rv_tr);

    // // top left
    // Camera* cam_tl = new Camera(*(new ViewingVolume()));
    // cam_tl->SetPosition(Vector<3,float>(dist,0,0));
    // cam_tl->LookAt(0,0,0);
    // Viewport* vp_tl = new Viewport(0,height/2, width/2,height);
    //     //new Viewport(800,600);
    // vp_tl->SetViewingVolume(cam_tl);
    // OpenGL::RenderingView* rv_tl = new RTRenderingView(*vp_tl);

    //config.renderer->ProcessEvent().Attach(*rv_tl);


    config.renderer->ProcessEvent().Attach(*rv);

    //config.ms->BindTool(config.viewport, tc);
    // config.ms->BindTool(vp_br, tc2);
    // config.ms->BindTool(vp_tl, tc);
    // config.ms->BindTool(vp_tr, tc3);

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

    ShapeNode *sn1 = new ShapeNode(new Shapes::Sphere(Vector<3,float>(20,10,-100), 15));
    sn1->shape->mat->diffuse = Vector<4,float>(1.0,0,0,1.0);
    sn1->shape->mat->specular = Vector<4,float>(1);
    sn1->shape->mat->shininess = 20;
    sn1->shape->reflection = 1.0;
    root->AddNode(sn1);

    ShapeNode *sn3 = new ShapeNode(new Shapes::Sphere(Vector<3,float>(-20,10,-100), 15));
    sn3->shape->mat->diffuse = Vector<4,float>(0,0,1.0,1.0);
    sn3->shape->reflection = .5;
    root->AddNode(sn3);

    // ShapeNode *sn3 = new ShapeNode(new Shapes::Ellipsoid(Vector<3,float>(0,0,0),
    //                                                      Vector<3,float>(1,1.0,,1)));
    // sn3->shape->mat->diffuse = Vector<4,float>(0,0,1.0,1.0);
    // sn3->shape->reflection = .5;
    // root->AddNode(sn3);


    ShapeNode *sn4 = new ShapeNode(new Shapes::Sphere(Vector<3,float>(0,10,-80), 15));
    sn4->shape->mat->diffuse = Vector<4,float>(0.005);
    sn4->shape->mat->specular = Vector<4,float>(1);
    sn4->shape->mat->shininess = 30;
    sn4->shape->transparent = true;
    sn4->shape->refraction = 2.42;
    sn4->shape->reflection = 0.2;
    
    root->AddNode(sn4);



    TransformationNode *tn2 = new TransformationNode();
    tn2->Move(0,-10,0);
    ShapeNode *sn2 = new ShapeNode(new Shapes::Plane(Vector<3,float>(0,-10,0),
                                                     Vector<3,float>(0,-10,1),
                                                     Vector<3,float>(1,-10,0)));
    tn2->AddNode(sn2);
    root->AddNode(tn2);




    config.traceTex = EmptyTextureResource::Create(800,600,24);
    config.traceTex->SetMipmapping(false);
    
    config.traceTex->Load();
    config.textureLoader->Load(config.traceTex, TextureLoader::RELOAD_QUEUED);

    //config.ms->SetScene(config.scene);
    config.textureLoader->Load(*config.scene);

    HUD::Surface *rtHud = config.hud->CreateSurface(config.traceTex);
    rtHud->SetPosition(HUD::Surface::LEFT,
                       HUD::Surface::TOP);
    //rtHud->SetScale(2.0,2.0);



    // Supply the scene to the renderer
    config.canvas->SetScene(config.scene);
    

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

// void SetupOpenCLhpp(Config& config) {
//     cl_int err;
//     int count = 1024;
//     float *data = new float[count];
//     float *results = new float[count];

//     for (int i=0;i<count;i++) 
//         data[i] = i;

//     vector<cl::Platform> platformList;
//     cl::Platform::get(&platformList);

//     logger.info << "CL: Found " << platformList.size() << " platforms" << logger.end;
    
//     cl::Platform platform = platformList[0];

//     string pvendor = platform.getInfo<CL_PLATFORM_VENDOR>();
//     logger.info << "CL: Vender = " << pvendor << logger.end;
    
//     vector<cl::Device> deviceList;
//     platform.getDevices(CL_DEVICE_TYPE_GPU, &deviceList);
    
//     logger.info << "CL: Found " << deviceList.size() << " devices" << logger.end;
//     for (vector<cl::Device>::iterator itr = deviceList.begin();
//          itr != deviceList.end();
//          itr++) {
//         cl::Device dev = *itr;
//         string devName;
//         devName = dev.getInfo<CL_DEVICE_NAME>();
//         logger.info << "CL: device name = " << devName << logger.end;
//     }
    
//     cl::Device device = deviceList[0];

//     cl::Context context(deviceList);

//     ifstream file("test1.cl");
//     string prog(istreambuf_iterator<char>(file),
//                 (istreambuf_iterator<char>()));

//     logger.info << "CL: prog = \n" << prog << logger.end;
    
//     cl::Program::Sources source(1, make_pair(prog.c_str(), prog.length()-1));
//     cl::Program program(context, source);
//     err = program.build(deviceList,"");
//     logger.info << "CL: Compile & Link = " << err << logger.end;
    
//     cl::Kernel kernel(program, "square", &err);
//     logger.info << "CL: Kernel = " << err << logger.end;
    
//     cl::Buffer input_buf(context, CL_MEM_READ_ONLY, sizeof(float)*count, NULL, &err);
//     logger.info << "CL: Buffer = " << err << logger.end;

//     cl::Buffer output_buf(context,CL_MEM_WRITE_ONLY,sizeof(float)*count,NULL,&err);
//     logger.info << "CL: Buffer = " << err << logger.end;
    
//     cl::CommandQueue queue(context,device,0,&err);
//     logger.info << "CL: Queue = " << err << logger.end;

//     err = queue.enqueueWriteBuffer(input_buf, CL_TRUE, 0, sizeof(float)*count, data);
//     logger.info << "CL: Write Buffer = " << err << logger.end;    

//     err = kernel.setArg(0,input_buf);
//     err = kernel.setArg(1,output_buf);
//     err = kernel.setArg(2,count);
//     logger.info << "CL: Args = " << err << logger.end;

//     cl::Event event;
//     err = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(count),
//                                      cl::NDRange(512),
//                                      NULL, &event);
//     logger.info << "CL: Queue Kernel = " << err << logger.end;
//     event.wait();
//     logger.info << "CL: Event done" << logger.end;

//     err = queue.enqueueReadBuffer(output_buf, CL_TRUE,0,sizeof(float)*count,results);
//     logger.info << "CL: Read Queue = " << err << logger.end;

//     for (int i=0;i<count;i++) {
//         logger.info << i << "^2 = " << results[i] << logger.end;
//     }


// }
// void SetupOpenCL(Config& config) {   
//     string file("test1.cl");
//     int size = File::GetSize(file);
//     ifstream* ifs = File::Open(file);
//     char *source = (char*)malloc(size);
//     ifs->read(source,size-1);
//     source[size-1] = '\0';
//     ifs->close();
//     delete ifs;

//     logger.info << "OpenCL:\n " << source << logger.end;
    
//     cl_device_id device_id;
//     cl_context context;
//     cl_command_queue queue;
//     cl_program program;
//     cl_kernel kernel;

//     cl_mem input;
//     cl_mem output;

//     size_t local;
//     size_t global;

//     unsigned int count = 1024;

//     float *data = new float[count];
//     float *results = new float[count];

//     for (int i=0;i<count;i++) 
//         data[i] = i;
    

//     int err;
//     err = clGetDeviceIDs(NULL, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
//     logger.info << "err = " << err << logger.end;

//     context = clCreateContext(0,1,&device_id,NULL,NULL,&err);
//     logger.info << "context = " << context << " err = " << err << logger.end;

//     queue = clCreateCommandQueue(context, device_id, 0, &err);
//     logger.info << "queue = " << queue << " err = " << err<< logger.end;

//     program = clCreateProgramWithSource(context, 1, (const char**)&source, NULL, &err);
//     logger.info << "program = " << program << " err = " << err << logger.end;
    
//     err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
//     logger.info << "err = " << err << logger.end;
    
//     kernel = clCreateKernel(program, "square", &err);
//     logger.info << "kernel = " << kernel << " err = " << err << logger.end;
    
//     input = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * count, NULL,NULL);
//     output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * count, NULL,NULL);
    
//     err = clEnqueueWriteBuffer(queue, input, CL_TRUE, 0, sizeof(float) * count, data, 0, NULL, NULL);
//     logger.info << "err = " << err << logger.end;

//     err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input);
//     err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &output);
//     err |= clSetKernelArg(kernel, 2, sizeof(unsigned int), &count);
//     logger.info << "err = " << err << logger.end;

//     err = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE,
//                                    sizeof(size_t), &local, NULL);

//     logger.info << "err = " << err << logger.end;

//     global = count;
//     logger.info << local << "," << global << logger.end;

//     err = clEnqueueNDRangeKernel(queue,kernel,1,NULL,&global,&local,0,NULL,NULL);
//     logger.info << "err[x] = " << err << logger.end;
    
//     clFinish(queue);

//     err = clEnqueueReadBuffer(queue,output,CL_TRUE,0,
//                               sizeof(float)*count, results,0,NULL,NULL);
//     logger.info << "err = " << err << logger.end;

//     clReleaseMemObject(input);
//     clReleaseMemObject(output);
//     clReleaseProgram(program);
//     clReleaseKernel(kernel);
//     clReleaseCommandQueue(queue);
//     clReleaseContext(context);

//     for (int i=0;i<count;i++) {
//         logger.info << i << "^2 = " << results[i] << logger.end;
//     }
// }
