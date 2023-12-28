#pragma once
#include <limits>
#include "vec.h"
#include "Triangle.h"
#include <vector>

struct Node {
    Vector min, max; // AXIS ALIGNED BOUNDING BOX (AABB)
    uint left; // right is left + 1
    uint begin, triangleCount;
    inline bool IsLeaf() const { return triangleCount > 0; }
    bool intersectBoundingBox(Ray & ray) const;
};

enum AXIS {
    X, Y, Z
};

struct BVH {
    std::vector<Node> nodes;
    std::vector<uint> idTrianglesBvh;
    std::vector<Triangle> * triangles;
    uint nodeUsed;
    uint idRoot;
    void UpdateBounds(uint idNode);
    void Subdivide(uint idNode);
    void Build(std::vector<Triangle> & triangles, uint idRoot = 0);
    void Intersect(Ray & ray, uint idNode = 0) const;
};

Vector vecMin(const Vector & a, const Vector & b) {
    Vector r;
    r.x = std::min(a.x, b.x);
    r.y = std::min(a.y, b.y);
    r.z = std::min(a.z, b.z);
    return r;
}

Vector vecMax(const Vector & a, const Vector & b) {
    Vector r;
    r.x = std::max(a.x, b.x);
    r.y = std::max(a.y, b.y);
    r.z = std::max(a.z, b.z);
    return r;
}

void BVH::UpdateBounds(uint idNode) {
    Node& node = nodes[idNode];
    node.min = Vector( std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() );
    node.max = Vector( std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min() );
    for (uint i = 0; i < node.triangleCount; i++)
    {
        Triangle& leafTri = triangles->at(idTrianglesBvh[node.begin + i]);
        node.min = vecMin( node.min, leafTri.getA() );
        node.min = vecMin( node.min, leafTri.getB() );
        node.min = vecMin( node.min, leafTri.getC() );
        node.max = vecMax( node.max, leafTri.getA() );
        node.max = vecMax( node.max, leafTri.getB() );
        node.max = vecMax( node.max, leafTri.getC() );
    }
}

void BVH::Subdivide(uint idNode) {
    // std::cout<<idNode << " / "<<nodes.size()<<std::endl;
    Node & node = nodes[idNode];
    if(node.triangleCount <= 2) return; // On ne subdivise pas les feuilles

    // On split au niveau du milieu de la plus grande dimension
    Vector size = node.max - node.min;
    AXIS axis = AXIS::X;
    if(size.y > size.x) axis = AXIS::Y;
    if(size.z > size(axis)) axis = AXIS::Z;
    float pivot = node.min(axis) + size(axis) * 0.5f;

    // Maintenant que l'axe est choisi, on split les triangles en deux
    // Les triangles dans prim count appartiennent a ce noeud, donc on a juste a les trier comme un quick sort
    // CAD -> Les triangles qui sont plus petits que mid sont a gauche, les plus grands a droite
    int i = node.begin;
    int j = i + node.triangleCount - 1;
    while(i <= j) {
        if(triangles->at(idTrianglesBvh[i]).getCentroid()(axis) < pivot) i++;
        else std::swap(idTrianglesBvh[i], idTrianglesBvh[j--]);
    }
    
    // On a maintenant deux groupes de triangles, on remplit les noeuds enfants
    uint leftCount = i - node.begin;
    if(leftCount == 0 || leftCount == node.triangleCount) return;

    node.left = nodeUsed + 1;
    nodeUsed += 2;

    nodes[node.left].begin = node.begin;
    nodes[node.left].triangleCount = leftCount;
    nodes[node.left + 1].begin = i;
    nodes[node.left + 1].triangleCount = node.triangleCount - leftCount;
    node.triangleCount = 0;
    UpdateBounds(node.left);
    UpdateBounds(node.left + 1);
    Subdivide(node.left);
    Subdivide(node.left + 1);
}

void BVH::Build(std::vector<Triangle> & t, uint idRoot) {
    triangles = &t;
    for(size_t i = 0; i < t.size(); i++) {
        idTrianglesBvh.push_back(i);
    }
    nodes.resize(t.size() * 2 - 1);
    nodeUsed = 1;
    Node & root = nodes[idRoot];
    root.left = 0;
    root.begin = 0;
    root.triangleCount = t.size();
    UpdateBounds(idRoot);
    Subdivide(idRoot);
}

void BVH::Intersect(Ray & ray, uint idNode) const {
    const Node & node = nodes[idNode];
    if(!node.intersectBoundingBox(ray)) return;
    if(node.IsLeaf()) {
        for(size_t i = node.begin; i < node.begin + node.triangleCount; i++) {
            triangles->at(idTrianglesBvh[i]).Intersect(ray);
        }
    } else {
        Intersect(ray, node.left);
        Intersect(ray, node.left + 1);
    }
}

bool Node::intersectBoundingBox(Ray & ray) const {
    float tx1 = (min.x - ray.origin.x) / ray.direction.x;
    float tx2 = (max.x - ray.origin.x) / ray.direction.x;
    float tmin = std::min( tx1, tx2 );
    float tmax = std::max( tx1, tx2 );

    float ty1 = (min.y - ray.origin.y) / ray.direction.y;
    float ty2 = (max.y - ray.origin.y) / ray.direction.y;
    tmin = std::max( tmin, std::min( ty1, ty2 ) );
    tmax = std::min( tmax, std::max( ty1, ty2 ) );

    float tz1 = (min.z - ray.origin.z) / ray.direction.z;
    float tz2 = (max.z - ray.origin.z) / ray.direction.z;
    tmin = std::max( tmin, std::min( tz1, tz2 ) );
    tmax = std::min( tmax, std::max( tz1, tz2 ) );

    return tmax >= tmin && tmin < ray.t && tmax > 0;
}