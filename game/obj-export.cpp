#include "obj-export.h"

#include <stdio.h>

static FILE* filePtr;
static u32 m_vertexBufferCounter;
static u32 m_indexBufferCounter;
static u32* m_verticesInBuffer;
static GLfloat m_scale;

void ObjExport_Start(const char* filename, const char* mapName, s16 sectorAmount, GLfloat scale)
{
    filePtr = fopen(filename, "w");
    m_vertexBufferCounter = 0;
    m_indexBufferCounter = 0;
    m_scale = scale;
    // One buffer for walls, two buffers for each sector (floor & ceiling)
    u32 bufferSize = 1 + sectorAmount * 2;
    m_verticesInBuffer = (u32*)mgdl_AllocateGraphicsMemory( bufferSize * sizeof(u32));
    for (u32 i = 0; i < bufferSize; i++)
    {
        m_verticesInBuffer[i] = 0;
    }

    fprintf(filePtr, "# Exported from %s\n", mapName);
}

void ObjExport_WriteVertices(GLfloat* vertices, u16 verticesCount, u16 vertexStride, s32 yOffset, u16 bufferIndex, char* name)
{
    int vi = 1;
    for (u32 ib = 0; ib < bufferIndex; ib++ )
    {
        vi += m_verticesInBuffer[ib];
    }
    fprintf(filePtr, "# Vertices buffer %d: %s. First %d. Count %d.\n", m_vertexBufferCounter, name, vi, verticesCount);
    for (u32 v = 0; v < verticesCount; v++)
    {
        int i = v * vertexStride;
        GLfloat x = vertices[i+0];
        GLfloat y = vertices[i+1];
        y += yOffset;
        GLfloat z = vertices[i+2];
        fprintf(filePtr, "v %f %f %f\n",
                x * m_scale,
                y * m_scale,
                z * m_scale);
    }
    m_verticesInBuffer[bufferIndex] += verticesCount;
    m_vertexBufferCounter += 1;
}

// NOTE Obj starts face numbering from 1
void ObjExport_WriteFaces(GLushort* indices, u32 indicesCount, u16 vertexBufferIndex, Obj_FaceWinding winding, char* name)
{

    fprintf(filePtr, "# Faces buffer #%d : %s\n", m_indexBufferCounter, name);
    // All index buffers start from 0, so need to offset the numbers
    // by the amount of vertices written before
    int i = 1;
    for (u32 ib = 0; ib < vertexBufferIndex; ib++ )
    {
        i += m_verticesInBuffer[ib];
    }
    fprintf(filePtr, "# Vertex offset %d\n", i);
    for (u32 f = 0; f < indicesCount; f+=3)
    {
        // Default order is CCW
        if (winding == Wind_CCW)
        {
            fprintf(filePtr, "f %d %d %d\n", i + indices[f+0], i + indices[f+1], i + indices[f+2]);
        }
        else if (winding == Wind_CW)
        {
            fprintf(filePtr, "f %d %d %d\n", i + indices[f+2], i + indices[f+1], i + indices[f+0]);
        }
    }
    m_indexBufferCounter += 1;
}

void ObjExport_Stop()
{
    fflush(filePtr);
    fclose(filePtr);
    mgdl_FreeGraphicsMemory(m_verticesInBuffer);
}
