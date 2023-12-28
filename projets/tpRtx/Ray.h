#pragma once
#include <limits>
#include "vec.h"
#include "Triangle.h"
#include <vector>


struct Ray
{
    // Origine du rayon
    Vector origin;
    // Direciton du rayon
    Vector direction;
    // Normal
    Vector normal;

    Color throughput = White();
    // Index du triangle intersecte
    int triangleIndex = -1;
    // Distance jusqu'a l'intersection
    float t = std::numeric_limits<float>::max();
    float u, v;         // p(u, v), position du point d'intersection sur le triangle


    Ray( const Point& origin, const Point& destination ) :  origin(origin), direction(Vector(origin, destination)) {}
    Ray(const Point & origin, const Vector & direction) : origin(origin), direction(direction) {}
};
