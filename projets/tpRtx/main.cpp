
//! \file tuto_rayons.cpp

#include <vector>
#include <cfloat>
#include <chrono>
#include <random>

#include <OpenImageDenoise/oidn.h>

#include "vec.h"
#include "mat.h"
#include "color.h"
#include "image.h"
#include "image_io.h"
#include "image_hdr.h"
#include "orbiter.h"
#include "mesh.h"
#include "wavefront.h"

#include "BVH.h"

#include "Scene.h"

#include <omp.h>

void denoise(Image & img, const std::vector<Color> & albedos, const std::vector<Vector> & normals) {
    OIDNDevice device = oidnNewDevice(OIDN_DEVICE_TYPE_DEFAULT);
    oidnCommitDevice(device);

    OIDNBuffer colorBuffer = oidnNewBuffer(device, img.width() * img.height() * sizeof(float) * 3);
    OIDNBuffer albedoBuffer = oidnNewBuffer(device, img.width() * img.height() * sizeof(float) * 3);
    OIDNBuffer normalBuffer = oidnNewBuffer(device, img.width() * img.height() * sizeof(float) * 3);
    
    OIDNFilter filter = oidnNewFilter(device, "RT");
    oidnSetFilterImage(filter, "color",  colorBuffer, OIDN_FORMAT_FLOAT3, img.width(), img.height(), 0, 0, 0); // beauty
    oidnSetFilterImage(filter, "albedo", albedoBuffer, OIDN_FORMAT_FLOAT3, img.width(), img.height(), 0, 0, 0); // beauty
    oidnSetFilterImage(filter, "normal", normalBuffer, OIDN_FORMAT_FLOAT3, img.width(), img.height(), 0, 0, 0); // beauty
    oidnSetFilterImage(filter, "output", colorBuffer, OIDN_FORMAT_FLOAT3, img.width(), img.height(), 0, 0, 0); // beauty denoised
    oidnSetFilter1b(filter, "hdr", true);
    oidnCommitFilter(filter);

    // fill color buffer
    float* colorPtr = (float*)oidnGetBufferData(colorBuffer);
    for (int i = 0; i < img.width() * img.height(); ++i)
    {
        colorPtr[3 * i + 0] = img(i).r;
        colorPtr[3 * i + 1] = img(i).g;
        colorPtr[3 * i + 2] = img(i).b;
    }

    // fill albedo buffer
    float* albedoPtr = (float*)oidnGetBufferData(albedoBuffer);
    for (int i = 0; i < img.width() * img.height(); ++i)
    {
        albedoPtr[3 * i + 0] = albedos[i].r;
        albedoPtr[3 * i + 1] = albedos[i].g;
        albedoPtr[3 * i + 2] = albedos[i].b;
    }

    // fill normal buffer
    float* normalPtr = (float*)oidnGetBufferData(normalBuffer);
    for (int i = 0; i < img.width() * img.height(); ++i)
    {
        normalPtr[3 * i + 0] = normals[i].x;
        normalPtr[3 * i + 1] = normals[i].y;
        normalPtr[3 * i + 2] = normals[i].z;
    }

    oidnExecuteFilter(filter);

    const char* errorMessage;
    if (oidnGetDeviceError(device, &errorMessage) != OIDN_ERROR_NONE)
        printf("Error: %s\n", errorMessage);    

    // fill image

    for (int i = 0; i < img.width() * img.height(); ++i)
    {
        img(i).r = colorPtr[3 * i + 0];
        img(i).g = colorPtr[3 * i + 1];
        img(i).b = colorPtr[3 * i + 2];
    }

    oidnReleaseFilter(filter);
    oidnReleaseBuffer(colorBuffer);
    oidnReleaseDevice(device);
}

#define NB_SAMPLES 256
#define PI_INV 1.0 / M_PI

Vector randomPointOnHemisphere(float u, float v) {
    float cos_theta = u;
    float sin_theta = sqrt(1 - u * u);
    float phi = 2 * M_PI * v;
    return Vector(cos(phi) * sin_theta, cos_theta, sin_theta * sin(phi));
}

Vector randomPointOnHemisphereCosinedWeighted(float r1, float r2, float & cos_theta) {
    float phi = 2 * M_PI * r1;
    float theta = acosf(sqrtf(r2));

    float pir1p2 = M_PI * r1 * 2;
    float s1r2 = sqrtf(1 - r2);

    cos_theta = cosf(theta);

    return Vector(cosf(pir1p2) * s1r2, sin(pir1p2) * s1r2, sqrtf(r2));
}

Vector getWorld(const Vector & T, const Vector & B, const Vector & N, const Vector & coordOnHemi) {
    return Vector(coordOnHemi.x * T.x + coordOnHemi.y * B.x + coordOnHemi.z * N.x,
                  coordOnHemi.x * T.y + coordOnHemi.y * B.y + coordOnHemi.z * N.y,
                  coordOnHemi.x * T.z + coordOnHemi.y * B.z + coordOnHemi.z * N.z);
}

const float eps = 1.0e-5f;

Color getDirect(Scene & scene, const Point & PO, const Vector & PN, std::default_random_engine & rng, std::uniform_real_distribution<float> & dist) {
    
    Color direct = Black();
    for(int i = 0; i < NB_SAMPLES; i++) {
        const Source & source = scene.sources[int(dist(rng) * scene.sources.size())];

        float pdf = (1 / float(scene.sources.size())) * (1 / source.area);
        float u = dist(rng);
        float v = dist(rng);
        
        Point O = PO + PN * eps;
        Point q = source.getPoint(u, v);
        Vector dir = normalize(q - O);

        Ray lray(O, dir);
        scene.Intersect(lray);
        Point I = Point(lray.origin + lray.direction * lray.t);
        int vis = length2(I - q) < eps;
        float cos_theta = std::max(0.f, dot(PN, dir));
        direct = direct + vis * scene.getMaterial(lray).emission * cos_theta * PI_INV * dot(source.Normal, -dir) / length2(q - O) / pdf;
    }

    return direct / float(NB_SAMPLES);
}

Color getIndirectCosinedWeigthed(Scene & scene, const Point & PO, const Vector & PN, std::default_random_engine & rng, std::uniform_real_distribution<float> & dist) {
    Color indirect = Black();
    for(int i = 0; i < NB_SAMPLES; i++) {
        float u = dist(rng);
        float v = dist(rng);
        float cos_theta_hemi;
        Vector pointOnHemi = randomPointOnHemisphereCosinedWeighted(u, v, cos_theta_hemi);

        float pdf = cos_theta_hemi * PI_INV;

        Vector T, B;
        if(abs(PN.x) > abs(PN.y)) {
            T = normalize(Vector(PN.z, 0, -PN.x));
        } else {
            T = normalize(Vector(0, -PN.z, PN.y));
        }
        B = normalize(cross(PN, T));

        Vector dirHemi = getWorld(T, B, PN, pointOnHemi);
        Ray lray(PO + PN * 1.0e-5, dirHemi);

        if(!scene.Intersect(lray)) {
            float cos_theta = std::max(0.f, dot(PN, dirHemi));
            indirect = indirect + White() * cos_theta / pdf;
        }
    }

    return indirect / float(NB_SAMPLES);
}

Color getIndirect(Scene & scene, const Point & PO, const Vector & PN, std::default_random_engine & rng, std::uniform_real_distribution<float> & dist) {
    Color indirect = Black();
    for(int i = 0; i < NB_SAMPLES; i++) {
        float u = dist(rng);
        float v = dist(rng);
        Vector pointOnHemi = randomPointOnHemisphere(u, v);

        float pdf = 1.0 / (2 * M_PI);

        Vector T, B;
        if(abs(PN.x) > abs(PN.y)) {
            T = normalize(Vector(PN.z, 0, -PN.x));
        } else {
            T = normalize(Vector(0, -PN.z, PN.y));
        }
        B = normalize(cross(PN, T));

        Vector dirHemi = getWorld(T, B, PN, pointOnHemi);
        Ray lray(PO + PN * 1.0e-5, dirHemi);
        float cos_theta = std::max(0.f, dot(PN, dirHemi));

        if(!scene.Intersect(lray)) {
            indirect = indirect + White() * cos_theta / pdf;
        }
    }

    return indirect / float(NB_SAMPLES);
}

Color getLight(Ray & ray, Scene & scene, Vector & PN, Color & albedo, std::default_random_engine & rng, std::uniform_real_distribution<float> & dist) {
    Color col = Black();
    if(scene.Intersect(ray)) { 

        const Material& mat = scene.getMaterial(ray);
        
        Point PO= Point(ray.origin + ray.direction * ray.t);
        PN = normalize(scene.normal(ray));
        albedo = mat.diffuse;
        
        Color direct = Black();
        //direct = getDirect(scene, PO, PN, rng, dist);
        
        Color indirect = Black();
        //indirect = getIndirect(scene, PO, PN, rng, dist);
        indirect = getIndirectCosinedWeigthed(scene, PO, PN, rng, dist);

        col = mat.emission + (direct + indirect) * mat.diffuse;
    } else {
        col = Color(0.4, 0.4, 0.8);
    }
    return col;
}

int main( const int argc, const char **argv )
{
    //const char *mesh_filename= "data/export.obj";
    const char * mesh_filename = "data/cornell.obj";
    if(argc > 1)
        mesh_filename= argv[1];

    //const char *orbiter_filename= "data/export_orbiter.txt";    
    const char *orbiter_filename= "data/cornell_orbiter.txt";
    if(argc > 2)
        orbiter_filename= argv[2];

    Scene scene(640, 360, orbiter_filename, mesh_filename);

    float w = 100;

    std::vector<std::default_random_engine> rngs;
    std::vector<std::uniform_real_distribution<float>> dists;
    for (size_t i = 0; i < omp_get_max_threads(); i++)
    {
        rngs.push_back(std::default_random_engine(i));
        dists.push_back(std::uniform_real_distribution<float>(0, 1));
    }

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<Vector> normals(scene.image.width() * scene.image.height(), Vector(0, 0, 0));
    std::vector<Color> albedos(scene.image.width() * scene.image.height(), Color(0, 0, 0));
    
    // c'est parti, parcours tous les pixels de l'image
    #pragma omp parallel for schedule(dynamic, 1)
    for(int y= 0; y < scene.image.height(); y++){
        for(int x= 0; x < scene.image.width(); x++)
        {
            Ray ray = scene.getPixelRay(x, y);
            Vector & PN = normals[y * scene.image.width() + x];
            Color & albedo = albedos[y * scene.image.width() + x];
            scene.image(x, y) = getLight(ray, scene, PN, albedo, rngs[omp_get_thread_num()], dists[omp_get_thread_num()]);
            if(x == 0) printf("x: %d / %d, y: %d / %d\n", x, scene.image.width(), y, scene.image.height());
        }
    }
    auto stop= std::chrono::high_resolution_clock::now();
    int cpu= std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    printf("%dms\n", cpu);
    
    denoise(scene.image, albedos, normals);

    write_image(scene.image, "render.png");
    write_image_hdr(scene.image, "shadow.hdr");

    
    return 0;
}
