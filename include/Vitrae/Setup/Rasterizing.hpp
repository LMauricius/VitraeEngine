#pragma once

#include "Vitrae/Data/Blending.hpp"
#include "Vitrae/Data/FragmentTest.hpp"
#include "Vitrae/Data/Typedefs.hpp"

namespace Vitrae
{

enum class CullingMode {
    None,
    Backface,
    Frontface
};

/**
 * @enum RasterizingMode
 * @brief Different rasterization modes
 * @details Specifies the method used for polygon rasterization.
 * Each option specifies several properties,
 * as there could be specifically tweaked settings and possibly specialized algorithms
 * for each one. The meaning of properties is as follows:
 * - Algorithm and feature control
 *      - Derivational: Organizes fragment shading in at least 2x2 blocks.
 *          Supports expression derivatives and automatic mipmap selection.
 *          Bad for small polygons. This is currently the only algorithm group supported,
 *          as any other would require a custom rasterizer to be made.
 * - Visualization
 *      - Fill: Draws filled polygons.
 *      - Trace: Draws outlines of polygons. Commonly called wireframe mode.
 *      - Dot: Draws by dotting vertices.
 * - Guides for choosing which fragments to produce
 *      - Centers: Produces fragments whose centers lie inside the polygon.
 *      - Edges: Produces fragments from edge to edge.
 *          Bordering fragments are interpolated as lying directly on the edges.
 *      - Vertices: Produces fragments from edge to edge and vertex to vertex.
 *          Bordering fragments are interpolated as lying directly on the edges,
 *          and the fragments closest to polygon vertices as the vertices.
 */
enum class RasterizingMode {
    DerivationalFillCenters,
    DerivationalFillEdges,
    DerivationalFillVertices,
    DerivationalTraceEdges,
    DerivationalTraceVertices,
    DerivationalDotVertices,
};

struct RasterizingSetupParams
{
    String vertexPositionOutputPropertyName;
    String modelFormPurpose;
    RasterizingMode rasterizingMode = RasterizingMode::DerivationalFillCenters;
    CullingMode cullingMode = CullingMode::Backface;
    BlendingDescription blending = BlendingCommon::None;
    FragmentTestFunction depthTest = FragmentTestFunction::LessOrEqual;
    float lineWidth = 1.0f;
    bool writeDepth : 1 = true;
    bool smoothFilling : 1 = false;
    bool smoothTracing : 1 = false;
    bool smoothDotting : 1 = false;
};

} // namespace Vitrae