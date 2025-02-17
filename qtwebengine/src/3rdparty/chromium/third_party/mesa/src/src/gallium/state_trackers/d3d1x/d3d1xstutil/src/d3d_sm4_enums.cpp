#include "d3d1xstutil.h"

unsigned d3d_to_pipe_prim[D3D_PRIMITIVE_TOPOLOGY_COUNT] =
{
	0,
	PIPE_PRIM_POINTS,
	PIPE_PRIM_LINES,
	PIPE_PRIM_LINE_STRIP,
	PIPE_PRIM_TRIANGLES,
	PIPE_PRIM_TRIANGLE_STRIP,
	PIPE_PRIM_LINES_ADJACENCY,
	PIPE_PRIM_LINE_STRIP_ADJACENCY,
	PIPE_PRIM_TRIANGLES_ADJACENCY,
	PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY,
	/* gap */
	0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0,
	/* patches */
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
};

unsigned d3d_to_pipe_prim_type[D3D_PRIMITIVE_COUNT] =
{
	0,
	PIPE_PRIM_POINTS,
	PIPE_PRIM_LINES,
	PIPE_PRIM_TRIANGLES,
	0,
	PIPE_PRIM_POINTS,
	PIPE_PRIM_LINES_ADJACENCY,
	PIPE_PRIM_TRIANGLES_ADJACENCY,
	/* patches */
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
};
