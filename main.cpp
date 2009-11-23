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

// SimpleSetup
#include <Utils/SimpleSetup.h>
#include <Geometry/FaceBuilder.h>
#include <Scene/GeometryNode.h>
#include <Scene/RenderStateNode.h>
#include <Scene/PointLightNode.h>
#include <Scene/TransformationNode.h>

#include <Script/Scheme.h>
#include <Script/ScriptBridge.h>

#include <Renderers/TextureLoader.h>
#include <Display/QtEnvironment.h>
#include <Display/Viewport.h>
#include <Renderers/OpenGL/RenderingView.h>
#include <Scene/ShapeNode.h>
#include <Shapes/Sphere.h>
#include <Shapes/Plane.h>

// Game factory
//#include "GameFactory.h"

#include "RayTracer.h"
#include "EmptyTextureResource.h"
    

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

static TransformationNode* CreateTextureBillboard(ITextureResourcePtr texture,
						  float scale) {
  unsigned int textureHosisontalSize = texture->GetWidth();
  unsigned int textureVerticalSize = texture->GetHeight();

  logger.info << "w x h = " << texture->GetWidth()
	      << " x " << texture->GetHeight() << logger.end;
  float fullxtexcoord = 1;
  float fullytexcoord = 1;
  
  FaceSet* faces = new FaceSet();

  float horisontalhalfsize = textureHosisontalSize * 0.5;
  Vector<3,float>* lowerleft = new Vector<3,float>(horisontalhalfsize,0,0);
  Vector<3,float>* lowerright = new Vector<3,float>(-horisontalhalfsize,0,0);
  Vector<3,float>* upperleft = new Vector<3,float>(horisontalhalfsize,textureVerticalSize,0);
  Vector<3,float>* upperright = new Vector<3,float>(-horisontalhalfsize,textureVerticalSize,0);

  FacePtr leftside = FacePtr(new Face(*lowerleft,*lowerright,*upperleft));

        /*
          leftside->texc[1] = Vector<2,float>(1,0);
          leftside->texc[0] = Vector<2,float>(0,0);
          leftside->texc[2] = Vector<2,float>(0,1);
        */
  leftside->texc[1] = Vector<2,float>(0,fullytexcoord);
  leftside->texc[0] = Vector<2,float>(fullxtexcoord,fullytexcoord);
  leftside->texc[2] = Vector<2,float>(fullxtexcoord,0);
  leftside->norm[0] = leftside->norm[1] = leftside->norm[2] = Vector<3,float>(0,0,1);
  leftside->CalcHardNorm();
  leftside->Scale(scale);
  faces->Add(leftside);

  FacePtr rightside = FacePtr(new Face(*lowerright,*upperright,*upperleft));
        /*
          rightside->texc[2] = Vector<2,float>(0,1);
          rightside->texc[1] = Vector<2,float>(1,1);
          rightside->texc[0] = Vector<2,float>(1,0);
        */
  rightside->texc[2] = Vector<2,float>(fullxtexcoord,0);
  rightside->texc[1] = Vector<2,float>(0,0);
  rightside->texc[0] = Vector<2,float>(0,fullytexcoord);
  rightside->norm[0] = rightside->norm[1] = rightside->norm[2] = Vector<3,float>(0,0,1);
  rightside->CalcHardNorm();
  rightside->Scale(scale);
  faces->Add(rightside);

  MaterialPtr m = leftside->mat = rightside->mat = MaterialPtr(new Material());
  m->texr = texture;

  GeometryNode* node = new GeometryNode();
  node->SetFaceSet(faces);
  TransformationNode* tnode = new TransformationNode();
  tnode->AddNode(node);
  return tnode;
}


/**
 * Main method for the first quarter project of CGD.
 * Corresponds to the
 *   public static void main(String args[])
 * method in Java.
 */

class RTRenderingView : public RenderingView {
public:
    RTRenderingView(Viewport& viewport) : IRenderingView(viewport),RenderingView(viewport) {}

    void VisitShapeNode(ShapeNode* node) {
        IRenderer *renderer = GetRenderer();
        
        Shape *shape = node->shape;

        Shapes::Sphere *sphere = dynamic_cast<Shapes::Sphere*>(shape);
        Shapes::Plane *plane = dynamic_cast<Shapes::Plane*>(shape);
        
        if (sphere) {
            renderer->DrawSphere(sphere->center, sphere->radius, Vector<3,float>(1,0,0));
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


int main(int argc, char** argv) {
    // Setup logging facilities.
    //Logger::AddLogger(new StreamLogger(&std::cout));

    // Print usage info.
    //logger.info << "========= Running OpenEngine Test Project =========" << logger.end;

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


