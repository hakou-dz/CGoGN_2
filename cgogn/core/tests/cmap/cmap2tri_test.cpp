/*******************************************************************************
* CGoGN: Combinatorial and Geometric modeling with Generic N-dimensional Maps  *
* Copyright (C) 2015, IGG Group, ICube, University of Strasbourg, France       *
*                                                                              *
* This library is free software; you can redistribute it and/or modify it      *
* under the terms of the GNU Lesser General Public License as published by the *
* Free Software Foundation; either version 2.1 of the License, or (at your     *
* option) any later version.                                                   *
*                                                                              *
* This library is distributed in the hope that it will be useful, but WITHOUT  *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License  *
* for more details.                                                            *
*                                                                              *
* You should have received a copy of the GNU Lesser General Public License     *
* along with this library; if not, write to the Free Software Foundation,      *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.           *
*                                                                              *
* Web site: http://cgogn.unistra.fr/                                           *
* Contact information: cgogn@unistra.fr                                        *
*                                                                              *
*******************************************************************************/

#include <gtest/gtest.h>

#include <cgogn/core/cmap/cmap2_tri_builder.h>

namespace cgogn
{

#define NB_MAX 100

/*!
 * \brief The CMap2TriTest class implements tests on embedded CMap2
 * It contains a CMap2 to which vertex, edge, face and volume attribute
 * are added to enforce the indexation mechanism in cell traversals.
 *
 * Note that pure topological operations have already been tested,
 * in CMap2TopoTest, thus only the indexation mechanism used for the
 * embedding of cells is tested here.
 */
class CMap2TriTest : public ::testing::Test
{

public:

	struct MiniMapTraits
	{
		static const uint32 CHUNK_SIZE = 16;
	};

	using testCMap2Tri = CMap2Tri<MiniMapTraits>;
	using MapBuilder = CMap2TriBuilder_T<MiniMapTraits>;
	using CDart = testCMap2Tri::CDart;
	using Vertex = testCMap2Tri::Vertex;
	using Edge = testCMap2Tri::Edge;
	using Face = testCMap2Tri::Face;
	using Volume = testCMap2Tri::Volume;

protected:

	testCMap2Tri cmap_;

	CMap2TriTest()
	{
	}

	void embed_map()
	{
		cmap_.add_attribute<int32, CDart::ORBIT>("darts");
		cmap_.add_attribute<int32, Vertex::ORBIT>("vertices");
		cmap_.add_attribute<int32, Edge::ORBIT>("edges");
		cmap_.add_attribute<int32, Face::ORBIT>("faces");
		cmap_.add_attribute<int32, Volume::ORBIT>("volumes");
	}

};

TEST_F(CMap2TriTest,tris)
{
	for (uint32 i=0; i<10; ++i)
	{
		cmap_.add_face(3);
	}
	EXPECT_TRUE(cmap_.check_map_integrity());
	EXPECT_EQ(cmap_.nb_cells<Vertex::ORBIT>(), 30);
	EXPECT_EQ(cmap_.nb_cells<Edge::ORBIT>(), 30);
	EXPECT_EQ(cmap_.nb_cells<Face::ORBIT>(), 10);
	EXPECT_EQ(cmap_.nb_cells<Volume::ORBIT>(), 10);


	embed_map();

	for (uint32 i=0; i<10; ++i)
	{
		cmap_.add_face(3);
	}
	EXPECT_TRUE(cmap_.check_map_integrity());
	EXPECT_EQ(cmap_.nb_cells<Vertex::ORBIT>(), 60);
	EXPECT_EQ(cmap_.nb_cells<Edge::ORBIT>(), 60);
	EXPECT_EQ(cmap_.nb_cells<Face::ORBIT>(), 20);
	EXPECT_EQ(cmap_.nb_cells<Volume::ORBIT>(), 20);

}

TEST_F(CMap2TriTest, builder)
{
	MapBuilder builder(cmap_);
	Dart d1 = builder.add_face_topo_parent(3);
	Dart d2 = builder.add_face_topo_parent(3);

	builder.phi2_sew(d1,d2);

	builder.close_map();

	EXPECT_TRUE(cmap_.check_map_integrity());
	EXPECT_EQ(cmap_.nb_cells<Vertex::ORBIT>(), 4);
	EXPECT_EQ(cmap_.nb_cells<Edge::ORBIT>(), 5);
	EXPECT_EQ(cmap_.nb_cells<Face::ORBIT>(), 2);
	EXPECT_EQ(cmap_.nb_cells<Volume::ORBIT>(), 1);
}


TEST_F(CMap2TriTest, flip)
{
	MapBuilder builder(cmap_);
	Dart d1 = builder.add_face_topo_parent(3);
	Dart d2 = builder.add_face_topo_parent(3);
	builder.phi2_sew(d1,d2);
	builder.close_map();

	embed_map();

	EXPECT_TRUE(cmap_.check_map_integrity());
	EXPECT_EQ(cmap_.degree(Vertex(d1)), 3);
	EXPECT_EQ(cmap_.degree(Vertex(d2)), 3);
	EXPECT_EQ(cmap_.degree(Vertex(cmap_.phi_1(d1))), 2);
	EXPECT_EQ(cmap_.degree(Vertex(cmap_.phi_1(d2))), 2);

	cmap_.flip_edge(Edge(d1));

	EXPECT_TRUE(cmap_.check_map_integrity());
	EXPECT_EQ(cmap_.degree(Vertex(d1)), 3);
	EXPECT_EQ(cmap_.degree(Vertex(d2)), 3);
	EXPECT_EQ(cmap_.degree(Vertex(cmap_.phi_1(d1))), 2);
	EXPECT_EQ(cmap_.degree(Vertex(cmap_.phi_1(d2))), 2);
	EXPECT_EQ(cmap_.nb_cells<Vertex::ORBIT>(), 4);
	EXPECT_EQ(cmap_.nb_cells<Edge::ORBIT>(), 5);
	EXPECT_EQ(cmap_.nb_cells<Face::ORBIT>(), 2);
	EXPECT_EQ(cmap_.nb_cells<Volume::ORBIT>(), 1);
}


TEST_F(CMap2TriTest, collapse)
{
	MapBuilder builder(cmap_);

	Dart d1 = builder.add_face_topo_parent(3);
	Dart d2 = builder.add_face_topo_parent(3);

	builder.phi2_sew(d1,d2);
	builder.close_hole(cmap_.phi1(d1));

	EXPECT_EQ(cmap_.nb_cells<Vertex::ORBIT>(), 5);
	EXPECT_EQ(cmap_.nb_cells<Edge::ORBIT>(), 9);
	EXPECT_EQ(cmap_.nb_cells<Face::ORBIT>(), 6);
	EXPECT_EQ(cmap_.nb_cells<Volume::ORBIT>(), 1);

	embed_map();

	EXPECT_TRUE(cmap_.check_map_integrity());
	EXPECT_EQ(cmap_.degree(Vertex(d1)), 4);
	EXPECT_EQ(cmap_.degree(Vertex(d2)), 4);
	EXPECT_EQ(cmap_.degree(Vertex(cmap_.phi_1(d1))), 3);
	EXPECT_EQ(cmap_.degree(Vertex(cmap_.phi_1(d2))), 3);


//	Vertex nv = cmap_.collapse_edge(Edge(d1));

//	EXPECT_TRUE(cmap_.check_map_integrity());
//	EXPECT_EQ(cmap_.degree(nv), 4);

//	EXPECT_EQ(cmap_.nb_cells<Vertex::ORBIT>(), 4);
//	EXPECT_EQ(cmap_.nb_cells<Edge::ORBIT>(), 6);
//	EXPECT_EQ(cmap_.nb_cells<Face::ORBIT>(), 4);
//	EXPECT_EQ(cmap_.nb_cells<Volume::ORBIT>(), 1);
}


TEST_F(CMap2TriTest, split_triangle)
{

	embed_map();
	Face f=cmap_.add_face(3);

	Vertex center = cmap_.split_triangle(f);

	EXPECT_TRUE(cmap_.check_map_integrity());

	EXPECT_EQ(cmap_.nb_cells<Vertex::ORBIT>(), 4);
	EXPECT_EQ(cmap_.nb_cells<Edge::ORBIT>(), 6);
	EXPECT_EQ(cmap_.nb_cells<Face::ORBIT>(), 3);
	EXPECT_EQ(cmap_.nb_cells<Volume::ORBIT>(), 1);
	EXPECT_EQ(cmap_.degree(center), 3);
}

TEST_F(CMap2TriTest, split_vertex)
{
	MapBuilder builder(cmap_);
	Dart d1 = builder.add_face_topo_parent(3);
	builder.close_hole(cmap_.phi1(d1));
	embed_map();

	EXPECT_EQ(cmap_.nb_cells<Vertex::ORBIT>(), 4);
	EXPECT_EQ(cmap_.nb_cells<Edge::ORBIT>(), 6);
	EXPECT_EQ(cmap_.nb_cells<Face::ORBIT>(), 4);
	EXPECT_EQ(cmap_.nb_cells<Volume::ORBIT>(), 1);

	EXPECT_TRUE(cmap_.check_map_integrity());

	Dart d2 = cmap_.phi2(cmap_.phi_1(d1));
	Edge e = cmap_.split_vertex(d1,d2);

	EXPECT_TRUE(cmap_.check_map_integrity());
	EXPECT_EQ(cmap_.nb_cells<Vertex::ORBIT>(), 5);
	EXPECT_EQ(cmap_.nb_cells<Edge::ORBIT>(), 9);
	EXPECT_EQ(cmap_.nb_cells<Face::ORBIT>(), 6);
	EXPECT_EQ(cmap_.nb_cells<Volume::ORBIT>(), 1);

	auto verts = cmap_.vertices(e);
	EXPECT_EQ(cmap_.degree(verts.first) + cmap_.degree(verts.second) , 7);
}

TEST_F(CMap2TriTest, cut_edge)
{
	MapBuilder builder(cmap_);

	Dart d1 = builder.add_face_topo_parent(3);
	Dart d2 = builder.add_face_topo_parent(3);

	builder.phi2_sew(d1,d2);
	builder.close_map();

	embed_map();

	Vertex nv = cmap_.cut_edge(Edge(d1));

	EXPECT_TRUE(cmap_.check_map_integrity());
	EXPECT_EQ(cmap_.degree(nv), 4);

	EXPECT_EQ(cmap_.nb_cells<Vertex::ORBIT>(), 5);
	EXPECT_EQ(cmap_.nb_cells<Edge::ORBIT>(), 8);
	EXPECT_EQ(cmap_.nb_cells<Face::ORBIT>(), 4);
	EXPECT_EQ(cmap_.nb_cells<Volume::ORBIT>(), 1);
}


TEST_F(CMap2TriTest, add_tetra)
{
	embed_map();
	Volume vol = cmap_.add_tetra();

	EXPECT_TRUE(cmap_.check_map_integrity());
	EXPECT_EQ(cmap_.nb_cells<Vertex::ORBIT>(), 4);
	EXPECT_EQ(cmap_.nb_cells<Edge::ORBIT>(), 6);
	EXPECT_EQ(cmap_.nb_cells<Face::ORBIT>(), 4);
	EXPECT_EQ(cmap_.nb_cells<Volume::ORBIT>(), 1);

	cmap_.foreach_incident_vertex(vol, [&] (Vertex v)
	{
		EXPECT_EQ(cmap_.degree(v), 3);
	});
}

} // namespace cgogn
