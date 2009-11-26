#include "RayTracer.h"
#include <Shapes/Sphere.h>
#include <Shapes/Ray.h>
#include <Math/Math.h>
#include <Scene/ISceneNode.h>
#include <Scene/ShapeNode.h>

template <unsigned int N, class T>
Vector<N,T> VecMult(Vector<N,T> a, Vector<N,T> b) {
    Vector<N,T> r;
    for (unsigned int i=0;i<N;i++) {
        r[i] = a[i] * b[i];
    }
    return r;
}

RayTracer::RayTracer(EmptyTextureResourcePtr tex, IViewingVolume* vol, ISceneNode* root)
    : texture(tex),traceNum(0),root(root),volume(vol),run(true) {
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
    l.pos = Vector<3,float>(200,100,100);
    l.color = Vector<4,float>(.5,.5,.5,1);
    lights.push_back(l);

    maxDepth = 1;

    timer.Start();

    markX = 200;
    markY = 80;

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

    rt->objectsLock.Lock();

    Shape *nearestObj = rt->NearestShape(r, p, markDebug);
    if (nearestObj) {
        int i=0;

        Ray lastRay = r;


        // for each ray, draw the ray to the light
        for (vector<Light>::iterator itr2 = rt->lights.begin();
             itr2 != rt->lights.end();
             itr2++) {
            rend->DrawLine(Line(lastRay.origin, itr2->pos ),
                           Vector<3,float>(0,0,0),2);

        }


        Vector<3,float> colors[5];
        colors[0] = Vector<3,float>(1,0,0);
        colors[1] = Vector<3,float>(0,1,0);
        colors[2] = Vector<3,float>(0,0,1);
        colors[3] = Vector<3,float>(1,1,0);
        colors[4] = Vector<3,float>(0,1,1);

        Line l(r.origin, p);
        rend->DrawLine(l,colors[i++%5],3);

        while (nearestObj && i < 5) {

            // second ray
            Ray reflectionRay;
            reflectionRay.origin = p;

            Vector<3,float> normal = nearestObj->NormalAt(p);
            Vector<3,float> d = r.direction;

            reflectionRay.direction = d - 2 * (normal * d ) * normal;

            rend->DrawLine(Line(p,p + normal*5.0),
                           Vector<3,float>(1,0,1),
                           2);

            rend->DrawLine(Line(reflectionRay.origin,reflectionRay.origin + reflectionRay.direction*10),
                           Vector<3,float>(.5),
                           2);

            nearestObj = rt->NearestShape(reflectionRay, p, markDebug);
            if (nearestObj)
                rend->DrawLine(Line(reflectionRay.origin, p),colors[i++%5],3);
            lastRay = reflectionRay;
            // for each ray, draw the ray to the light
            for (vector<Light>::iterator itr2 = rt->lights.begin();
                 itr2 != rt->lights.end();
                 itr2++) {
                rend->DrawLine(Line(lastRay.origin, itr2->pos ),
                               Vector<3,float>(0,0,0),2);

            }
        }


    }
    rt->objectsLock.Unlock();
    markDebug = false;
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

Shape* RayTracer::NearestShape(Ray r, Vector<3,float>& point, bool debug) {

    float nearestT = 10000000000000000;
    Shape *nearestObj = 0;
    Vector<3,float> nearestPoint;


    for (vector<Object>::iterator itr = objects.begin();
         itr != objects.end();
         itr++) {
        Shape* s = itr->shape;

        Vector<3,float> interP;

        Hit h = s->Intersect(r,interP);

        if (h == HIT_OUT) {
            float t = (interP - r.origin).GetLength();
            if (debug)
                logger.info << " t = " << t << logger.end;
            if ( t < nearestT && t > 0) {
                nearestT = t;
                nearestObj = s;
                nearestPoint = interP;
            }
        }
    }

    if (nearestObj) {

        if (debug) {
            logger.info  << " best t = " << nearestT 
                         << " => " << nearestObj
                         << logger.end;
        }


        point = nearestPoint;
        return nearestObj;
    }

    if (debug)
        logger.info  <<" best t = 0" << logger.end;


    return 0;
}


Vector<4,float> RayTracer::TraceRay(Ray r, int depth, bool debug) {
    if (depth > maxDepth)
        return Vector<4,float>();

    Shape *nearestObj = 0;
    Vector<3,float> nearestPoint;


    if (debug)
        logger.info << "Depth = " << depth << logger.end;

    // Intersect all objects

    nearestObj = NearestShape(r,nearestPoint,debug);

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
            if (s2->Intersect(shaddowRay, shaddowInterP) == HIT_OUT) {

                float dist = (shaddowInterP - shaddowRay.origin).GetLength();
                float distToLight = (l.pos - shaddowRay.origin).GetLength();
                if (dist < distToLight)
                    isShaddow = true;

            }
        }

        if (!isShaddow) {

            Vector<3,float> norm = nearestObj->NormalAt(nearestPoint);
            float diff = (shaddowRay.direction * norm );


            if (diff > 0)
                color += diff * (l.color  * nearestObj->mat->diffuse);

            if (debug) {
                logger.info << "diff = " << diff << logger.end;
                logger.info << "diffuse = " << nearestObj->mat->diffuse << logger.end;
                logger.info << "color = " 
                            << diff * VecMult(l.color, nearestObj->mat->diffuse) 
                            << logger.end;
            }


            //color = l.color;
        } else {
            if(debug)
                logger.info << "shaddow!" << logger.end;

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

    
    reflectionRay.AddEps();
    
    Vector<4,float> recurseColor = TraceRay(reflectionRay,depth+1,debug);
    float reflection = 0.9;

    if (debug)
        logger.info << "reflection color = " 
                    << recurseColor * reflection 
                    << logger.end;

    color += recurseColor * reflection;

    //return recurseColor;

    for (int i=0;i<4;i++) {
        color[i] = fmin(color[i],1.0);
    }

    //(*texture)(x,y,traceNum % 3) = x*2;
    //color.Normalize();

    return color;
}

Vector<3,float> TransformPoint(Vector<3,float> p, Matrix<4,4,float> m) {
    Vector<3,float> r;

    Vector<4,float> pp(p[0],
                       p[1],
                       p[2],
                       1);
    m.Transpose();
    m = m.GetInverse();
    pp = (m * pp);

    
    r = Vector<3,float>(pp[0],
                        pp[1],
                        pp[2]);
    

    return r;
}

Ray RayTracer::RayForPoint(int u, int v) {

    float x = ((2*u - width) / width) * tan(fovX);
    float y = ((2*v - height) / height) * tan(fovY);

    Vector<3,float> p;
    p[0] = x;
    p[1] = y;
    p[2] = -1;

    // Camera    
    Matrix<4,4,float> projMat = volume->GetProjectionMatrix();
    Matrix<4,4,float> viewMat = volume->GetViewMatrix();


    if (markDebug &&
        markX == u &&
        markY == v) {
        logger.info << u << " " << v << logger.end;
        logger.info << p << logger.end;
        projMat.Transpose();
        logger.info << projMat.GetInverse() * Vector<4,float>(2*u/width,2*v/height,1,1) << logger.end;
    }




    Vector<3,float> cp = camPos;

    cp = TransformPoint(cp,viewMat);
    p = TransformPoint(p,viewMat);




    Ray r;
    r.origin = cp;
    r.direction = (p - cp).GetNormalize();

    return r;

}

void RayTracer::Trace() {
    Timer t;
    t.Reset();
    t.Start();

    // lock

    objectsLock.Lock();
    objects.clear();
    root->Accept(*this);
    objectsLock.Unlock();

    // Camera    
    Matrix<4,4,float> projMat = volume->GetProjectionMatrix();
    Matrix<4,4,float> viewMat = volume->GetViewMatrix();

    for (int i=0;i<4;i++) {
        for(int j=0;j<4;j++) {
            logger.info << projMat[i][j] << ",";
        }
        logger.info << logger.end;
    }

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

            Vector<4,float> col;
            if (markX == u && markY == v)
                col = TraceRay(r,0,markDebug);
            else 
                col = TraceRay(r,0);

            
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

    logger.info << "Trace time: " << t.GetElapsedTime() << logger.end;

    markDebug = false;
}

void RayTracer::Run() {

    while (run) {
        Trace();
        Thread::Sleep(1000000);
    }

}

RayTracer::RayTracerRenderNode* RayTracer::GetRayTracerDebugNode() {
    return rnode;
}
