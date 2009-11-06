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

// Game factory
//#include "GameFactory.h"

#include "RayTracer.h"

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


/**
 * Main method for the first quarter project of CGD.
 * Corresponds to the
 *   public static void main(String args[])
 * method in Java.
 */

int main(int argc, char** argv) {
    // Setup logging facilities.
    //Logger::AddLogger(new StreamLogger(&std::cout));

    // Print usage info.
    //logger.info << "========= Running OpenEngine Test Project =========" << logger.end;

    // Create simple setup
    SimpleSetup* setup = new SimpleSetup("Example Project Title");
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
    root->AddNode(gn);


    FaceSet *fs1 = new FaceSet();
    state.color = Vector<4,float>(0,1,0,1);
    
    FaceBuilder::MakeASphere(fs1, state, Vector<3,float>(-20,0,0), 20, 10);
    GeometryNode *gn2 = new GeometryNode(fs1);
    TransformationNode *sphereTrans = new TransformationNode();
    sphereTrans->SetPosition(Vector<3,float>(10,10,10));
    sphereTrans->AddNode(gn2);
    root->AddNode(sphereTrans);

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
    cam->SetPosition(Vector<3,float>(100,100,-100));
    cam->LookAt(Vector<3,float>(0,0,0));
    


    RayTracerPtr rt = RayTracer::Create();
    HUD::Surface *rtHud = setup->GetHUD().CreateSurface(rt);
    rtHud->SetPosition(HUD::Surface::LEFT,
                       HUD::Surface::TOP);



    // Start the engine.
    setup->GetEngine().Start();

    // Return when the engine stops.
    return EXIT_SUCCESS;
}


