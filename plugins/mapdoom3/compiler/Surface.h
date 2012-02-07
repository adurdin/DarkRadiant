#pragma once

#include <vector>
#include <map>
#include "render/ArbitraryMeshVertex.h"
#include "math/AABB.h"

namespace map
{

class Surface
{
private:
	// typedefs needed to simulate the idHashIndex class
	typedef std::multimap<int, std::size_t> IndexLookupMap;
	typedef std::pair<typename IndexLookupMap::const_iterator, 
					  typename IndexLookupMap::const_iterator> Range;

	struct SilEdge
	{
		// NOTE: making this a glIndex is dubious, as there can be 2x the faces as verts
		int	p1, p2;					// planes defining the edge
		int	v1, v2;					// verts defining the edge
	};

	IndexLookupMap _silEdgeLookup;

	static std::size_t MAX_SIL_EDGES;

	std::size_t _numDuplicatedEdges;
	std::size_t _numTripledEdges;
	std::size_t	_numPlanes;
	std::size_t _numSilEdges;

	static std::size_t _totalCoplanarSilEdges;
	static std::size_t _totalSilEdges;

public:
	AABB		bounds;

	typedef std::vector<ArbitraryMeshVertex> Vertices;
	Vertices	vertices;

	typedef std::vector<int> Indices;
	Indices		indices;

	// indexes changed to be the first vertex with same XYZ, ignoring normal and texcoords
	Indices		silIndexes;

	typedef std::vector<SilEdge> SilEdges;
	SilEdges	silEdges;

	// true if there aren't any dangling edges
	bool		perfectHull;

	// mirroredVerts[0] is the mirror of vertices.size() - mirroredVerts.size() + 0
	std::vector<int> mirroredVerts;

	// pairs of the number of the first vertex and the number of the duplicate vertex
	std::vector<int> dupVerts;

	Surface() :
		perfectHull(false)
	{}

	void calcBounds();

	void cleanupTriangles(bool createNormals, bool identifySilEdges, bool useUnsmoothedTangents);

private:
	// Check for syntactically incorrect indexes, like out of range values.
	// Does not check for semantics, like degenerate triangles.
	// No vertexes is acceptable if no indexes.
	// No indexes is acceptable.
	// More vertexes than are referenced by indexes are acceptable.
	bool rangeCheckIndexes();

	// Uniquing vertexes only on xyz before creating sil edges reduces
	// the edge count by about 20% on Q3 models
	void createSilIndexes();

	std::vector<int> createSilRemap();

	void removeDegenerateTriangles();

	void testDegenerateTextureSpace();

	// If the surface will not deform, coplanar edges (polygon interiors)
	// can never create silhouette plains, and can be omited
	void identifySilEdges(bool omitCoplanarEdges);
	void defineEdge(int v1, int v2, int planeNum);
	static int SilEdgeSort(const void* a_, const void* b_);

	// Modifies the surface to bust apart any verts that are shared by both positive and
	// negative texture polarities, so tangent space smoothing at the vertex doesn't
	// degenerate.
	// 
	// This will create some identical vertexes (which will eventually get different tangent
	// vectors), so never optimize the resulting mesh, or it will get the mirrored edges back.
	// 
	// Reallocates vertices and changes indices in place
	// Silindexes are unchanged by this.
	// 
	// sets mirroredVerts and mirroredVerts[]
	void duplicateMirroredVertexes();

	// Returns true if the texture polarity of the face is negative, false if it is positive or zero
	bool getFaceNegativePolarity(std::size_t firstIndex) const;

	void createDupVerts();
};

} // namespace
