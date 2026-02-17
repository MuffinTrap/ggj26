#pragma once
#include <mgdl.h>

#ifdef __cplusplus
extern "C" {
#endif

    enum Obj_FaceWinding
    {
        Wind_CW,
        Wind_CCW
    };
    typedef enum Obj_FaceWinding Obj_FaceWinding;

    void ObjExport_Start(const char* filename, const char* mapname, s16 sectorAmount, GLfloat scale);
    void ObjExport_WriteVertices(GLfloat* vertices, u16 verticesCount, u16 vertexStride, s32 yOffset, u16 bufferIndex, char* name);
    /**
     * @brief Writes faces that refer to vertices written earlier
     * @param indices Buffer of vertex indices
     * @param indicesCount Size of indices buffer
     * @param vertexBufferIndex What vertex buffer do this faces refer to
     * @param winding How to wind the faces.
     * @param name Identifier written as a comment to the obj file
     */
    void ObjExport_WriteFaces(GLushort* indices, u32 indicesCount, u16 vertexBufferIndex, Obj_FaceWinding winding, char* name);
    void ObjExport_Stop();

#ifdef __cplusplus
}
#endif
