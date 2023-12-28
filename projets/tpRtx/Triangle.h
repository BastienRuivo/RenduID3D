#pragma once

#include "vec.h"
#include "Ray.h"
#include "mat.h"
#include "materials.h"
#include <iostream>


const float EPSILON = 1.0e-4;

class Triangle
{
private:
    Vector A, B, C, AB, AC, Centroid;
    int index, matIndex;
    
public:
    Triangle(Vector A, Vector B, Vector C, int index) : A(A), B(B), C(C), index(index) {
        this->AB = B - A;
        this->AC = C - A;
        this->Centroid = (A + B + C) / 3.0;
    }

    Triangle(Vector A, Vector B, Vector C, Vector normal, int index) : A(A), B(B), C(C), index(index) {
        this->AB = B - A;
        this->AC = C - A;
        this->Centroid = (A + B + C) / 3.0;
    }

    Triangle& applyTransform(const Transform & t) {
        A = Vector(t(Point(A)));
        B = Vector(t(Point(B)));
        C = Vector(t(Point(C)));
        AB = B - A;
        AC = C - A;
        Centroid = (A + B + C) / 3.0;
        return *this;
    }

    inline int getIndex() const {
        return index;
    }

    inline float minX() const {
        return std::min(A.x, std::min(B.x, C.x));
    }

    inline float maxX() const {
        return std::max(A.x, std::max(B.x, C.x));
    }

    inline float minY() const {
        return std::min(A.y, std::min(B.y, C.y));
    }

    inline float maxY() const {
        return std::max(A.y, std::max(B.y, C.y));
    }

    inline float minZ() const {
        return std::min(A.z, std::min(B.z, C.z));
    }

    inline float maxZ() const {
        return std::max(A.z, std::max(B.z, C.z));
    }

    inline Vector getA() const {
        return A;
    }
    inline Vector getB() const {
        return B;
    }
    inline Vector getC() const {
        return C;
    }
    inline Vector getAB() const {
        return AB;
    }

    inline Triangle& operator* (const Transform & t) {
        return applyTransform(t);
    }

    inline Vector getAC() const {
        return AC;
    }
    inline int getMatIndex() const {
        return matIndex;
    }

    inline void setA(Vector A) {
        this->A = A;
    }
    inline void setB(Vector B) {
        this->B = B;
    }
    inline void setC(Vector C) {
        this->C = C;
    }
    inline void setMatIndex(int matIndex) {
        this->matIndex = matIndex;
    }

    inline Vector getCentroid() const {
        return Centroid;
    }

    const Color & diffuse_color(const Mesh & mesh) const {
        return mesh.materials().material(matIndex).diffuse;
    }

    

    inline bool isInside(Vector barycentrics) const {
        return (barycentrics.x >= 0 && barycentrics.y >= 0 && barycentrics.z >= 0) || (barycentrics.x <= 0 && barycentrics.y <= 0 && barycentrics.z <= 0);
    }

    Vector getBarycentric(Vector P) const {
        // Compute vectors
        Vector res;
        res.x = ((B.y - C.y) * (P.x - C.x) + (C.x - B.x) * (P.y - C.y)) / ((B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y));
        res.y = ((C.y - A.y) * (P.x - C.x) + (A.x - C.x) * (P.y - C.y)) / ((B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y));
        res.z = 1.0f - res.x - res.y;
        return res;
    }

    bool Intersect(Ray & ray, const float tmax = std::numeric_limits<float>::max()) const {
        // Cross direction with AC, Vector perpendicular to AC and ray direction
        const Vector dirAC = cross( ray.direction, AC );
        
        // Check if ray is parallel to triangle plans, which means that the dot product of plan normal and ray direction is 0
        const float a = dot( AB, dirAC );
        if (a > -EPSILON && a < EPSILON) return false; // ray parallel to triangle

        // If we get here, the ray is not parallel to the triangle plans, and may intersect the triangle
        const float f = 1 / a; // 1 / dot( AB, dirAC )
        const Vector s = ray.origin - A; //  Vector from A to ray origin
        const float u = f * dot( s, dirAC ); // barycentric coordinate between ABC and ray, if it's greather than 1, the intersection is outside the triangle
        // If u is negative, the intersection is behind AC 
        if (u < 0 || u > 1) return false; // intersection is outside triangle

        // Do the same for the other two edges

        // Cross direction with AB, Vector perpendicular to AB and ray direction
        const Vector q = cross( s, AB ); // Vector perpendicular to AB and ray direction
        const float v = f * dot( ray.direction, q ); // barycentric coordinate between ABC and ray, if it's greather than 1, the intersection is outside the triangle
        if (v < 0 || u + v > 1) return false; // intersection is outside triangle

        // Compute t, distance between ray origin and intersection point
        const float t = f * dot( AC, q );
        if (t > EPSILON && t < ray.t) {
            ray.t = t;
            ray.triangleIndex = index;
            ray.normal = (1 - u - v) * A + u * B + v * C;
            ray.u = u;
            ray.v = v;
        }

        return t < tmax;

        // Vector pvec= cross(ray.direction, AC);
        // float det= dot(AB, pvec);
        
        // float inv_det= 1 / det;
        // Vector tvec(A, ray.origin);
        
        // float u= dot(tvec, pvec) * inv_det;
        // if(u < EPSILON || u > 1) return false;
        
        // Vector qvec= cross(tvec, AB);
        // float v= dot(ray.direction, qvec) * inv_det;
        // if(v < EPSILON || u + v > 1) return false;
        
        // float t= dot(AC, qvec) * inv_det;
        // if(t > tmax || t < EPSILON) return false;

        // ray.t = t;
        // ray.u = u;
        // ray.v = v;
        // ray.triangleIndex = index;

        
        // return true;
    }
};
