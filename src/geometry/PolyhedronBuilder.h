#include <CGAL/Polyhedron_3.h>
#include <CGAL/Polyhedron_incremental_builder_3.h>
#include <CGAL/Polyhedron_items_with_id_3.h>

namespace pepr3d {

typedef CGAL::Polyhedron_3<pepr3d::DataTriangle::K, CGAL::Polyhedron_items_with_id_3> Polyhedron;
typedef Polyhedron::HalfedgeDS HalfedgeDS;

/// Builds a CGAL Polyhedron from triangles
template <class HDS>
class PolyhedronBuilder : public CGAL::Modifier_base<HDS> {
   private:
    const std::vector<std::array<size_t, 3>>& mTriangles;
    const std::vector<glm::vec3>& mVertices;

    std::vector<typename CGAL::Polyhedron_incremental_builder_3<HDS>::Face_handle> mFacetsCreated;

   public:
    PolyhedronBuilder(const std::vector<std::array<size_t, 3>>& tris, const std::vector<glm::vec3>& verts)
        : mTriangles(tris), mVertices(verts) {}

    std::vector<typename CGAL::Polyhedron_incremental_builder_3<HDS>::Face_handle> getFacetArray() {
        assert(mFacetsCreated.size() > 0 && mFacetsCreated.size() == mTriangles.size());
        return mFacetsCreated;
    }

    void operator()(HDS& hds) {
        // Postcondition: hds is a valid polyhedral surface.
        CGAL::Polyhedron_incremental_builder_3<HDS> B(hds, true);

        mFacetsCreated.clear();
        mFacetsCreated.reserve(mTriangles.size());

        // We will create a surface with <triangle-size> faces and <vertex-size> vertices
        B.begin_surface(mVertices.size(), mTriangles.size());

        typedef typename HDS::Vertex Vertex;
        typedef typename Vertex::Point Point;

        // Add all vertices
        for(const auto& vertex : mVertices) {
            B.add_vertex(Point(vertex.x, vertex.y, vertex.z));
        }

        for(const auto& tri : mTriangles) {
            auto facetBeingAdded = B.begin_facet();
            mFacetsCreated.push_back(facetBeingAdded);
            B.add_vertex_to_facet(tri[0]);
            B.add_vertex_to_facet(tri[1]);
            B.add_vertex_to_facet(tri[2]);
            B.end_facet();
        }

        B.end_surface();

        assert(mFacetsCreated.size() == mTriangles.size());
    }
};
}  // namespace pepr3d