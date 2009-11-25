#include "RayTracer.h"
#include <Shapes/Sphere.h>
#include <Shapes/Ray.h>
#include <Math/Math.h>
#include <Scene/ISceneNode.h>
#include <Scene/ShapeNode.h>

RayTracer::RayTracer(EmptyTextureResourcePtr tex, ISceneNode* root) : texture(tex),traceNum(0),root(root),run(true) {
    for (unsigned int x=0;x<texture->GetWidth();x++) 
        for (unsigned int y=0;y<texture->GetHeight();y++) {
            (*texture)(x,y,0) = x;
            (*texture)(x,y,1) = y;
            (*texture)(x,y,2) = -1;
            (*texture)(x,y,3) = -1;
        }
    height = tex->GetHeight();
    width = tex->GetWidth();

    fovX = PI/4.0; // ~ 60 deg
    fovY = height/width * fovX;

           
    camPos = Vector<3,float>(0,0,0);

    Light l;
    l.pos = Vector<3,float>(0,400,400);
    l.color = Vector<4,float>(1,1,1,1);
    lights.push_back(l);

    maxDepth = 1;

    timer.Start();

    markX = 180;
    markY = 15;

    rnode = new RayTracerRenderNode(this);
}

void RayTracer::RayTracerRenderNode::Apply(IRenderingView *rv) {
    IRenderer* rend = rv->GetRenderer();

    // draw the cam
    rend->DrawPoint(rt->camPos, Vector<3,float>(0,0,1), 10);

    // Draw a ray

    int u,v;

    u = rt->markX;
    v = rt->markY;

    Ray r = rt->RayForPoint(u,v);
    Vector<3,float> p;
    Shape *nearestObj = rt->NearestShape(r, p);
    
    int i=0;

    Vector<3,float> colors[5];
    colors[0] = Vector<3,float>(1,0,0);
    colors[1] = Vector<3,float>(0,1,0);
    colors[2] = Vector<3,float>(0,0,1);
    colors[3] = Vector<3,float>(1,1,0);
    colors[4] = Vector<3,float>(0,1,1);

    Line l(r.origin, p);
        rend->DrawLine(l,colors[i++%5],3);

    while (nearestObj) {               
    
        // second ray
        Ray reflectionRay;
        reflectionRay.origin = p;
            
        Vector<3,float> normal = nearestObj->NormalAt(p);
        Vector<3,float> d = r.direction;
            
        reflectionRay.direction = d - 2 * (normal * d ) * normal;
     
        nearestObj = rt->NearestShape(reflectionRay, p);
        if (nearestObj)
            rend->DrawLine(Line(reflectionRay.origin, p),colors[i++%5],3);
                  
    }
}


void RayTracer::VisitShapeNode(ShapeNode* node) {
    Object o;
    o.shape = node->shape;
    objects.push_back(o);
}

void RayTracer::Handle(ProcessEventArg arg) {

    if (dirty && timer.GetElapsedIntervals(100000)) {
        
        timer.Reset();
        texture->RebindTexture();
        dirty = false;
    }


}
/*

  From Wikipedia: http://en.wikipedia.org/wiki/Ray_tracing_(graphics)

For each pixel in image {
  Create ray from eyepoint passing through this pixel
  Initialize NearestT to INFINITY and NearestObject to NULL

  For every object in scene  {
     If ray intersects this object {
        If t of intersection is less than NearestT {
           Set NearestT to t of the intersection
           Set NearestObject to this object
        }
     }
  }

  If NearestObject is NULL {
     Fill this pixel with background color
  } Else {
     Shoot a ray to each light source to check if in shadow
     If surface is reflective, generate reflection ray: recurse
     If surface is transparent, generate refraction ray: recurse
     Use NearestObject and NearestT to compute shading function
     Fill this pixel with color result of shading function
  }
}

*/

Shape* RayTracer::NearestShape(Ray r, Vector<3,float>& point) {

    float nearestT = 10000000000000000;
    Shape *nearestObj = 0;
    Vector<3,float> nearestPoint;


    for (vector<Object>::iterator itr = objects.begin();
         itr != objects.end();
         itr++) {
        Shape* s = itr->shape;
        
        Vector<3,float> interP;

        bool i = s->Intersect(r,interP);
                
        if (i) {                    
            float t = (interP - r.origin).GetLength();
            if ( t < nearestT) {
                nearestT = t;
                nearestObj = s;
                nearestPoint = interP;
            }                    
        }
    }
    if (nearestObj) {
        point = nearestPoint;
        return nearestObj;
    }
    return 0;
}


Vector<4,float> RayTracer::TraceRay(Ray r, int depth) {
    if (depth > maxDepth)
        return Vector<4,float>();

    Shape *nearestObj = 0;
    Vector<3,float> nearestPoint;


            
    // Intersect all objects

    nearestObj = NearestShape(r,nearestPoint);

    if (!nearestObj)
        return Vector<4,float>(0,0,0,1);

    //logger.info << "distance " << nearestT << logger.end;

    // check if shadow.
    // Intersect lights
    Vector<4,float> color = Vector<4,float>(0,0,0,1);
    for (vector<Light>::iterator itr2 = lights.begin();
         itr2 != lights.end();
         itr2++) {
        Light l = *itr2;        

        bool isShaddow = false;
        Ray shaddowRay;
        shaddowRay.origin = nearestPoint;

        Vector<3,float> lineToLight = (l.pos - nearestPoint);
 
        shaddowRay.direction = lineToLight.GetNormalize();

        for (vector<Object>::iterator itr = objects.begin();
             itr != objects.end();
             itr++) {
            
            Shape *s2 = itr->shape;

            if (s2 == nearestObj)
                continue;
            

            Vector<3,float> shaddowInterP;
            if (s2->Intersect(shaddowRay, shaddowInterP)) {

                float dist = (shaddowInterP - shaddowRay.origin).GetLength();
                float distToLight = (l.pos - shaddowRay.origin).GetLength();
                if (dist < distToLight)
                    isShaddow = true;

            }

        }
    
        if (!isShaddow) {

            Vector<3,float> norm = nearestObj->NormalAt(nearestPoint);
            float diff = (norm * shaddowRay.direction);
            
            if (diff > 0)
                color += l.color * diff * nearestObj->mat->diffuse;

            //color = l.color;
        }
    }
    
    

    //return color;


    
    Ray reflectionRay;
    reflectionRay.origin = nearestPoint;
    Vector<3,float> normal = nearestObj->NormalAt(nearestPoint);
    Vector<3,float> d = r.direction;

    reflectionRay.direction = d - 2 * (normal * d ) * normal;

    //logger.info << "normal " << normal << logger.end;
    //logger.info << "reflection " << reflectionRay << logger.end;
    

    Vector<4,float> recurseColor = TraceRay(reflectionRay,depth+1);    
    float reflection = 0.9;


    color += recurseColor * reflection; 

    //return recurseColor;
    
    for (int i=0;i<4;i++) {
        color[i] = fmin(color[i],1.0);
    }    

    //(*texture)(x,y,traceNum % 3) = x*2;
    //color.Normalize();

    return color;
}

Ray RayTracer::RayForPoint(int u, int v) {

    float x = ((2*u - width) / width) * tan(fovX);
    float y = ((2*v - height) / height) * tan(fovY);
            
    Vector<3,float> p;
    p[0] = x;
    p[1] = y;
    p[2] = -1;

    Ray r;
    r.origin = camPos;
    r.direction = (p - camPos).GetNormalize();
    
    return r;

}

void RayTracer::Trace() {
    Timer t;
    t.Reset();
    t.Start();

    objects.clear();

    root->Accept(*this);

    traceNum++;
    for (unsigned int v=0;v<texture->GetHeight();v++) {
        for (unsigned int u=0;u<texture->GetWidth();u++) {

            (*texture)(u,v,0) = -1;
            (*texture)(u,v,1) = 0;
            (*texture)(u,v,2) = 0;
            (*texture)(u,v,3) = -1;


            // Create ray from eyepoint passing throuth this pixel


            Ray r = RayForPoint(u,v);
        
            //logger.info << "Ray: " << r << logger.end;
    
            Vector<4,float> col = TraceRay(r,0);

            (*texture)(u,v,0) = col[0]*255;
            (*texture)(u,v,1) = col[1]*255;
            (*texture)(u,v,2) = col[2]*255;
            //(*texture)(u,v,3) = col[3]*255;
            
            //logger.info << "color " << col << logger.end;
            //Thread::Sleep(50000);

            if (markX == u && markY == v) {
                (*texture)(u,v,0) = -1;
                (*texture)(u,v,1) = 0;
                (*texture)(u,v,2) = 0;
            }

            dirty = true;
        }
        dirty = true;
        //Thread::Sleep(100000);
    }

    dirty = true;
    t.Stop();

    logger.info << "time: " << t.GetElapsedTime() << logger.end;
    
    logger.info << "Trace" << logger.end;
}

void RayTracer::Run() {
    
    while (run) {
        Trace();
        Thread::Sleep(1000000);
    }

}

RenderNode* RayTracer::GetRayTracerDebugNode() {
    return rnode;
}
