#pragma once

#include <orbiter.h>
#include <mesh.h>
#include "Triangle.h"
#include "Ray.h"
#include <image.h>
#include <mesh.h>
#include <wavefront.h>
#include "BVH.h"
#include <chrono>
#include <map>

class Source
{
    public:
    Point A, B, C;
    Vector AB, AC, Normal;
    Color emission;
    float area;

    Source(Point a, Point b, Point c, Color emission) : A(a), B(b), C(c), emission(emission) {
        AB = B - A;
        AC = C - A;
        area = length(cross(AB, AC)) / 2;
        Normal = normalize(cross(AB, AC));
    }


    Point getPoint(float u, float v) const {
        float squ = sqrt(u);
        Point bary(1 - squ, squ * (1 - v), squ * v);
        return Point(A * bary.x + B * bary.y + C * bary.z);
    }

    Point getCentroid() const {
        return (A + B + C) / 3.0;
    }
};

class Scene
{
private:
public:
    Orbiter camera;
    Mesh mesh;
    std::vector<Triangle> triangles;
    std::vector<Source> sources;
    Image image;

    float areaSource;

    Transform model, view, projection, viewport, mvpInv;

    BVH bvh;

    Scene(int w, int h, const char * orbiter_filename, const char * mesh_filename) {
        image = Image(w, h);
        camera.read_orbiter(orbiter_filename);
        camera.projection(image.width(), image.height(), 45);

        model = Identity();
        view = camera.view();
        projection = camera.projection();
        viewport = camera.viewport();
        mvpInv = Inverse(viewport * projection * view * model);

        setMesh(mesh_filename);
        initBvh();
    }

    void setMesh(const char * mesh_filename) {
        mesh= read_mesh(mesh_filename);
        // recupere les triangles
        int n= mesh.triangle_count();
        for(int i= 0; i < n; i++)
        {
            const TriangleData& data= mesh.triangle(i);
            triangles.push_back( Triangle(data.a, data.b, data.c, i));

            const Material& material= mesh.triangle_material(i);
            if(material.emission.r + material.emission.g + material.emission.b > 0)
            {
                const TriangleData& data= mesh.triangle(i);
                sources.push_back( Source(data.a, data.b, data.c, material.emission) );
            }
        }

        float area = 0;

        std::cout<<areaSource<<std::endl;
        
        n= mesh.triangle_count();
        printf("%d sources\n", int(sources.size()));
        assert(sources.size() > 0);
    }

    void initBvh() {
        auto start= std::chrono::high_resolution_clock::now();

        bvh.Build(triangles);

        auto end= std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed= end - start;
        printf("BVH built in %fs\n", elapsed.count());
    }
    
    bool IntersectBVH(Ray& ray, float tmax = std::numeric_limits<float>::max()) {
        bvh.Intersect(ray);
        return ray.t < tmax;
    }

    bool Intersect(Ray& ray, float tmax = std::numeric_limits<float>::max()) const {
        for (size_t i = 0; i < triangles.size(); i++)
        {
            triangles[i].Intersect(ray);
        }
        
        return ray.t < tmax;
    }

    Ray getPixelRay(int x, int y) const {
        Point origine= mvpInv(Point(x + .5f, y + .5f, 0));
        Point extremite= mvpInv(Point(x + .5f, y + .5f, 1));
        return Ray(origine, extremite);
    }
    
    Vector normal(const Ray& hit)
    {
        // recuperer le triangle complet dans le mesh
        const TriangleData& data= mesh.triangle(hit.triangleIndex);
        Vector ab = Vector(data.b) - Vector(data.a);
        Vector ac = Vector(data.c) - Vector(data.a);
        // Make n into the screen space
        return normalize(model(cross(ab, ac)));
    }

    const Material & getMaterial(const Ray& hit )
    {
        if(hit.triangleIndex < 0)
            return mesh.materials().default_material();
        else {
            const Material & material= mesh.triangle_material(hit.triangleIndex);
            return material;
        }
    }
    
    ~Scene() {}
};