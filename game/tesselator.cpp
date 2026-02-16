#include "tesselator.h"
#include "mgdl/ccVector/ccVector.h"

GLUtesselator* tesselator = nullptr;
#define VERTEX_BUFFER_VERTEX_SIZE 5

// These are given as parameters
static GLfloat* vertexBuffer = nullptr;
static GLushort* indexBuffer = nullptr;
static u32 vertexBufferSize = 0;
static u32 indexBufferSize = 0;

// Tesselation counting
static u32 vertexBufferVertexIndex = 0;
static u32 indexBufferIndex = 0;
static RectF activeUVLimits;

#ifdef MGDL_PLATFORM_WINDOWS
#define _GLUfuncptr void(*)()
#endif

// TESSELATION CALLBACKS
// /////////////////////

// Ring buffer for tesselation input
// This buffer needs to hold all the vertices of a sector floor or ceiling
// for tesselation, so it is large
#define TESSELATION_BUFFER_SIZE_DOUBLES (3*128) // Divisible by three for the ring buffering to work; 3 doubles per vertex
#define TESSELATION_BUFFER_SIZE_BYTES (TESSELATION_BUFFER_SIZE_DOUBLES * sizeof(double))
static GLdouble* tesselationBuffer = nullptr;
static int tesselationBufferIndexDoubles = 0;

// Ring buffer for tesselation combine
// This is needed if two vertices are identical, but hopefully it is not needed
#define COMBINE_BUFFER_SIZE_DOUBLES (3*9) // Divisible by three for the ring buffering to work; 3 doubles per vertex
#define COMBINE_BUFFER_SIZE_BYTES (COMBINE_BUFFER_SIZE_DOUBLES * sizeof(double))
static GLdouble* combineRingBuffer = nullptr;
static int CombineBufferIndexDoubles = 0;

/**
 * @brief Put a vertex in the output buffer and set the indice of it in index buffer
 */
static void BufferVertex(const float x, const float y, const float z, const float u, const float v)
{
    Log_InfoF("Buffer vertex got C(%.2f %.2f, %.2f), TX(%.2f, %.2f)\n", x, y, z, u, v);
    static const float tolerance = 0.9f; // Duke units are integers, so this can be quite large
    static const float uvTolerance = 0.001f; // This is way smaller because values usually are under 10
    // Is this vertex already in the buffer?
    bool found = false;
    GLushort index = 0;
    // Incoming vertex
    vec2 V = vec2New(x,z);
    vec2 TX = vec2New(u,v);
    for (int i = 0; i < vertexBufferVertexIndex; i++)
    {
        GLfloat* vertex = &vertexBuffer[i * VERTEX_BUFFER_VERTEX_SIZE];

        // Existing vertex
        vec2 ex = vec2New(vertex[0], vertex[2]);
        float d = vec2Length( vec2Subtract(ex, V));
        if (d < tolerance)
        {
            vec2 exTx = vec2New(vertex[3], vertex[4]);
            float dtx = vec2Length( vec2Subtract(exTx, TX));
            if (dtx < uvTolerance)
            {
                Log_InfoF("Found at index %d\n", i);
                found = true;
                index = i;
                break;
            }
        }
    }
    if (found)
    {
        if (indexBufferIndex < indexBufferSize)
        {
            indexBuffer[indexBufferIndex] = index;
            indexBufferIndex++;
        }
        else
        {
            indexBufferIndex++;
            Log_ErrorF("Tesselator ran out of space in the index buffer, needs at least %d indices\n", indexBufferIndex);
        }
    }
    else
    {
        Log_InfoF("New vertex to index %d\n", vertexBufferVertexIndex);
        mgdl_assert_print(vertexBufferSize > vertexBufferVertexIndex, "Tesselator ran out of space in vertex buffer");
        GLfloat* vertex = &vertexBuffer[vertexBufferVertexIndex * VERTEX_BUFFER_VERTEX_SIZE];
        vertex[0] = x;
        vertex[1] = y;
        vertex[2] = z;

        vertex[3] = activeUVLimits.x + u * activeUVLimits.w;
        vertex[4] = activeUVLimits.y + v * activeUVLimits.h;


        if (indexBufferIndex < indexBufferSize)
        {
            indexBuffer[indexBufferIndex] = vertexBufferVertexIndex;
            indexBufferIndex++;
        }
        else
        {
            indexBufferIndex++;
            Log_ErrorF("Tesselator ran out of space in the index buffer, needs at least %d indices\n", indexBufferIndex);
        }
        vertexBufferVertexIndex += 1;
    }
}


#ifndef CALLBACK
#define CALLBACK
#endif

void CALLBACK tessBegin(GLenum which)
{
    //Log_InfoF("Tesselation start mode: %s \n", which == GL_TRIANGLES ? "Triangles" : "Not triangles");
}

// This puts a new vertex into the buffer: called after gluTessEndPolygon
void CALLBACK tessVertex(GLvoid* vertex)
{

    const GLdouble* coordinates = (GLdouble*)vertex;
    // Log_InfoF("Tesselation vertex C(%.2f %.2f, %.2f), TX(%.2f, %.2f, %.2f)\n", coordinates[0], coordinates[1], coordinates[2], coordinates[3], coordinates[4], coordinates[5]);
    BufferVertex(
        (GLfloat)coordinates[0], (GLfloat)coordinates[1], (GLfloat)coordinates[2],
        (GLfloat)coordinates[3], (GLfloat)coordinates[4]);
}

void CALLBACK tessCombine(GLdouble coords[3], GLdouble* vertex_data[4], GLfloat weight[4], GLdouble **dataOut)
{
    //Log_InfoF("Tesselation combine vertex: %.2f, %.2f\n", coords[0], coords[2]);
    if (CombineBufferIndexDoubles + 6 >= COMBINE_BUFFER_SIZE_DOUBLES)
    {
        CombineBufferIndexDoubles = 0;
    }
    // Reads 6 doubles
    GLdouble* vertex = &combineRingBuffer[CombineBufferIndexDoubles];

    // Coordinates of the combined vertex
    vertex[0] = coords[0];
    vertex[1] = coords[1];
    vertex[2] = coords[2];
    vertex[3] = 0.0f;
    vertex[4] = 0.0f;
    vertex[5] = 0.0f;
    /*  This causes crashes so don't do it
     *    for (int i = 3; i < 6; i++)
     *    {
     *        vertex[i] = weight[0] * vertex_data[0][i] +
     *                    weight[1] * vertex_data[1][i] +
     *                    weight[2] * vertex_data[2][i] +
     *                    weight[3] * vertex_data[3][i];
}
*/
    *dataOut = vertex;
    CombineBufferIndexDoubles = (CombineBufferIndexDoubles + 6) % COMBINE_BUFFER_SIZE_DOUBLES;
}

void CALLBACK tessEnd(void)
{

}

void CALLBACK tessError(GLenum errorCode)
{
    const GLubyte* str;
    str = gluErrorString(errorCode);
    Log_ErrorF("Tesselation error: %s\n", str);
}

void CALLBACK tessEdgeFlag(GLboolean flag)
{
    #ifndef GEKKO
    glEdgeFlag(flag);
    #endif
}

void Tesselator_Init()
{
    if (tesselator == nullptr)
    {
        tesselator = gluNewTess();
        mgdl_assert_print(tesselator != nullptr, "No Glut tesselator!");

        gluTessCallback(tesselator, GLU_TESS_BEGIN, (_GLUfuncptr)tessBegin);
        gluTessCallback(tesselator, GLU_TESS_VERTEX, (_GLUfuncptr)tessVertex);
        gluTessCallback(tesselator, GLU_TESS_END, (_GLUfuncptr)tessEnd);
        gluTessCallback(tesselator, GLU_TESS_ERROR, (_GLUfuncptr)tessError);
        gluTessCallback(tesselator, GLU_TESS_EDGE_FLAG, (_GLUfuncptr)tessEdgeFlag); // this makes tess only submit triangles
        gluTessCallback(tesselator, GLU_TESS_COMBINE, (_GLUfuncptr)tessCombine);
        if (tesselationBuffer == nullptr)
        {
            tesselationBuffer = (GLdouble*)mgdl_AllocateGraphicsMemory(TESSELATION_BUFFER_SIZE_BYTES);
        }
        if (combineRingBuffer == nullptr)
        {
            combineRingBuffer = (GLdouble*)mgdl_AllocateGraphicsMemory(COMBINE_BUFFER_SIZE_BYTES);
        }
    }
}

void Tesselator_SetBuffers(GLfloat* vertices, u32 verticesSize, GLushort* indices, u32 indicesSize)
{
    vertexBuffer = vertices;
    vertexBufferSize = verticesSize;
    indexBuffer = indices;
    indexBufferSize = indicesSize;

    vertexBufferVertexIndex = 0;
    indexBufferIndex = 0;
    mgdl_assert_print(indexBuffer != nullptr && vertexBuffer != nullptr, "Tesselator received null pointers for buffer addresses");
}
/**
 * @brief Returns the starting index in vertices buffer
 */
Tesselator_BufferIndices Tesselator_BeginPolygon(GLfloat normal[3], RectF uvLimits)
{
    mgdl_assert_print(indexBuffer != nullptr && vertexBuffer != nullptr, "Tesselator has no buffers set, cannot start polygon");

    activeUVLimits = uvLimits;
    gluTessNormal(tesselator, normal[0], normal[1], normal[2]);
    gluTessBeginPolygon(tesselator, NULL);

    tesselationBufferIndexDoubles = 0; // Start from beginning

    Tesselator_BufferIndices indices;
    indices.indexCount = 0;
    indices.indexIndex = indexBufferIndex;
    return indices;
}
void Tesselator_BeginContour()
{
    gluTessBeginContour(tesselator);

}
void Tesselator_EndContour()
{
    gluTessEndContour(tesselator);

}
void Tesselator_AddVertexToPoly(GLfloat vertex[3], GLfloat uv[2])
{

    // Tesselation
    // NOTE DANGER Must be counter clockwise
    //Log_InfoF("Tesselation vertex AddToPoly C(%.2f %.2f, %.2f), TX(%.2f, %.2f)\n", vertex[0], vertex[1], vertex[2], uv[0], uv[1]);
    // TODO Send normal too, but maybe not with every vertex?
    tesselationBuffer[tesselationBufferIndexDoubles + 0] = vertex[0];
    tesselationBuffer[tesselationBufferIndexDoubles + 1] = vertex[1];
    tesselationBuffer[tesselationBufferIndexDoubles + 2] = vertex[2];
    tesselationBuffer[tesselationBufferIndexDoubles + 3] = uv[0];
    tesselationBuffer[tesselationBufferIndexDoubles + 4] = uv[1];
    tesselationBuffer[tesselationBufferIndexDoubles + 5] = 0.0f;

    // NOTE  Always put the same address for both, even when their data is different
    // glutess does the pointer arithmetic itself.
    gluTessVertex(tesselator,
                  &tesselationBuffer[tesselationBufferIndexDoubles], &tesselationBuffer[tesselationBufferIndexDoubles]);

    /*
    Log_InfoF("Tesselation buffer at %d C(%.2f %.2f, %.2f), TX(%.2f, %.2f, %.2f)\n",
              tesselationBufferIndexDoubles,
              tesselationBuffer[tesselationBufferIndexDoubles+0], tesselationBuffer[tesselationBufferIndexDoubles+1], tesselationBuffer[tesselationBufferIndexDoubles+2],
              tesselationBuffer[tesselationBufferIndexDoubles+3], tesselationBuffer[tesselationBufferIndexDoubles+4], tesselationBuffer[tesselationBufferIndexDoubles+5] );
              */


    tesselationBufferIndexDoubles = (tesselationBufferIndexDoubles + 6) % TESSELATION_BUFFER_SIZE_DOUBLES;

}
/**
 * @brief Returns the amount of triangles in the polygon
 */
Tesselator_BufferIndices Tesselator_EndPolygon()
{
    mgdl_CacheFlushRange(tesselationBuffer, TESSELATION_BUFFER_SIZE_BYTES);
    gluTessEndPolygon(tesselator);
    Tesselator_BufferIndices indices;
    indices.indexIndex = indexBufferIndex;
    // Log_InfoF("Tesselator end polygon to vertex %d, index %d\n", vertexBufferVertexIndex, indexBufferIndex);
    return indices;
}

void Tesselator_Deinit()
{
    gluDeleteTess(tesselator);
    mgdl_FreeGraphicsMemory(tesselationBuffer);
    mgdl_FreeGraphicsMemory(combineRingBuffer);
}
