#ifndef _GLSL_H
#define _GLSL_H


#include "mesh.h"

class GLSL {
    public:
    struct alignas(16) Vec3 {
        float x, y, z;
    };

    struct Object {
        Vec3 pmin;
        Vec3 pmax;
        unsigned int index_count;
    };

    static Object makeObject(const TriangleGroup & g) {
        Object o = {
            {g.min.x, g.min.y, g.min.z}, 
            //unsigned(g.n),
            {g.max.x, g.max.y, g.max.z},
            unsigned(g.n)
        };
        return o;
    }

    static Object makeObject(const Point & min, const Point & max, unsigned int n) {
        Object o = {
            {min.x, min.y, min.z}, 
            //unsigned(g.n),
            {max.x, max.y, max.z},
            unsigned(n)
        };
        return o;
    }

    // representation des parametres de multidrawELEMENTSindirect
    struct alignas(4) IndirectParam
    {
        unsigned index_count;
        unsigned instance_count;
        unsigned first_index;
        unsigned vertex_base;
        unsigned instance_base;
    };

    static IndirectParam makeIndirectParam(const TriangleGroup & g) {
        IndirectParam p = {
            unsigned(g.n),      // count
            1,                          // instance_count
            unsigned(g.first),  // first_index
            0,                          // vertex_base, pas la peine de renumeroter les sommets
            0                           // instance_base, pas d'instances
        };
        return p;
    }

    static IndirectParam makeIndirectParam(unsigned count, unsigned first) {
        IndirectParam p = {
            count,      // count
            1,                          // instance_count
            first,  // first_index
            0,                          // vertex_base, pas la peine de renumeroter les sommets
            0                           // instance_base, pas d'instances
        };
        return p;
    }

    struct alignas(16) MaterialGPU
    {
        Color diffuse;              //!< couleur diffuse / de base.
        // Color specular;             //!< couleur du reflet.
        // Color emission;             //!< pour une source de lumiere.
        float ns;                   //!< concentration des reflets, exposant pour les reflets blinn-phong.
        int diffuse_texture;        //!< indice de la texture de la couleur de base, ou -1.
        // int specular_texture;        //!< indice de la texture, ou -1.
        // int emission_texture;        //!< indice de la texture, ou -1.
        int ns_texture;             //!< indice de la texture de reflet, ou -1.
    };

}; // namespace GLSL

#endif // _GLSL_H