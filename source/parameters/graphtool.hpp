#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphml.hpp>
//#include <graph.hh>

#include "entities/entity.hpp"
#include "entities/demographicprofile.hpp"
#include "utility/time.hpp"

//using namespace graph_tool;

/**
   This is the interface with graph_tool https://graph-tool.skewed.de/
 **/

// define enums in Boost
#if 0
namespace boost {
    enum vertex_orientation_t { vertex_orientation_msw, vertex_orientaiton_msmw, vertex_orientaiton_msm };
    enum edge_type_t { edge_type_steady, edge_type_regular, edge_type_casual, edge_type_csw };

    BOOST_INSTALL_PROPERTY(vertex, orientation);
    BOOST_INSTALL_PROPERTY(edge, type);
}
#endif

class Network {

    struct EntityVertex
    {
        int id;
        //boost::vertex_orientation_t orientation;
    };

    struct PartnershipEdge
    {
        int type;
        //boost::edge_type_t type;
        //Time duration;
    };

    // Adjacency List
    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                                  EntityVertex, PartnershipEdge> Graph;
    typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;
    typedef boost::graph_traits<Graph>::edge_descriptor edge_t;

public:
    Network() {}

#if 0
    // Edges are added when a partnership is formed
    void AddEdges(/*sexualPartnership*/)
    {
        PartnershipEdge edge(/*edge_properties, entityId, partnerId*/);
        boost::add_edge(edge);
    }

    // Edges are removed when a partnership ends
    void RemoveEdges()
    {
        PartnershipEdge edge(/*edge_properties, entityId, partnerId*/);
        boost::remove_edge(edge);
    }

    // New Vertices are added for entities that reach age of majority
    void AddVertices()
    {
        // for new vertices:
        EntityVertex vertex(/*vertex_properties*/);
        boost::add_vertex(vertex);
    }

    // Vertices are removed when an entity dies or ages out of the sexually active population
    void RemoveVertices(/*entityId*/)
    {
        // IndexMap index = get(vertex_index, g);
        EntityVertex vertex = get(/*entityId*/);
        boost::remove_vertex(vertex);
    }

    void UpdateNetwork()
    {
        //RemoveEdges();
        //RemoveVertices();
        //AddVertices();
        //AddEdges();
    }
#endif

    void UpdatePartnership(int entityId, int partnerId, int type)
    {
        // Create two vertices in that graph
        vertex_t u = boost::add_vertex(EntityVertex{entityId}, G);
        vertex_t v = boost::add_vertex(EntityVertex{partnerId}, G);

        // Create an edge conecting those two vertices
        boost::add_edge(u, v, PartnershipEdge{type}, G);
    }

    void Write()
    {
        boost::dynamic_properties dp;
        dp.property("id", get(&EntityVertex::id, G));
        dp.property("type", get(&PartnershipEdge::type, G));

        std::ofstream ofs("ParntershipNetwork.graphml");
        boost::write_graphml(ofs, G, dp, true);
#if 0
        boost::python::object ovprops;
        boost::python::object oeprops;
        boost::python::object vorder;

        GraphInterface network(G, true, ovprops, oeprops, vorder);
        network.write_to_file("ParternshipNetwork.gt", nullptr,
            "gt", graph.get_graph_index());
#endif
    }

private:
    Graph G;

};

