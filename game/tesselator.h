#pragma once
#include <mgdl.h>

#ifdef __cplusplus
    extern "C" {
#endif

    struct Tesselator_BufferIndices
    {
        u16 indexIndex;
        u16 indexCount;

        // Needed for obj export
        u16 vertexIndex;
        u16 vertexCount;
    };
    typedef struct Tesselator_BufferIndices Tesselator_BufferIndices;

void Tesselator_Init();

void Tesselator_SetBuffers(GLfloat* vertices, u32 verticeSize, GLushort* indices, u32 indicesSize);
/**
 * @brief Returns the indices before polygon
 */
Tesselator_BufferIndices Tesselator_BeginPolygon(GLfloat normal[3], RectF uvLimits);
void Tesselator_AddVertexToPoly(GLfloat vertex[3], GLfloat uv[2]);
void Tesselator_BeginContour();
void Tesselator_EndContour();
/**
 * @brief Returns the indices after polygon
 */
Tesselator_BufferIndices Tesselator_EndPolygon();

void Tesselator_Deinit();

#ifdef __cplusplus
}
#endif
