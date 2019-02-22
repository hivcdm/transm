#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphml.hpp>
//#include <graph.hh>

#include "entities/entity.hpp"
#include "entities/demographicprofile.hpp"
#include "entities/sexualpartnership.hpp"
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
        bool hiv_pos;
        bool on_prep;
        bool on_ART;
        bool risk_level;
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
    typedef boost::graph_traits<Graph>::vertex_iterator vertex_iter_t;
    typedef boost::graph_traits<Graph>::edge_descriptor edge_t;

    vertex_iter_t FindVertex(const Graph& g, int id)
    {
        vertex_iter_t vi, vi_end;
        for (boost::tie(vi, vi_end) = boost::vertices(g); vi != vi_end; ++vi)
        {
            if(g[*vi].id == id)
                return vi;
        }
        return vi_end;
    }

public:
    Network() {}

    void UpdatePartnership(transm::Entity *entity, transm::Entity *partner,
        transm::SexualPartnership *partnership)
    {
        vertex_t entity_vertex, partner_vertex;
        edge_t partnership_edge;
        bool inserted;

        vertex_iter_t vertex_iter;
        vertex_iter = FindVertex(G, entity->getID());
        if (vertex_iter == boost::vertices(G).second)
        {
            entity_vertex = add_vertex(G);
            G[entity_vertex].id = entity->getID();
            G[entity_vertex].hiv_pos = (entity->getHIVStatus() != transm::Entity::HIVStatus::NEGATIVE);
            G[entity_vertex].on_prep = entity->UsingPrEP();
            G[entity_vertex].on_ART = entity->isOnArt();
            G[entity_vertex].risk_level = (int)entity->getRiskLevel();

        } else {
            entity_vertex = *vertex_iter;
        }

        vertex_iter = FindVertex(G, partner->getID());
        if (vertex_iter == boost::vertices(G).second)
        {
            partner_vertex = add_vertex(G);
            G[partner_vertex].id = partner->getID();
            G[partner_vertex].hiv_pos = (partner->getHIVStatus() != transm::Entity::HIVStatus::NEGATIVE);
            G[partner_vertex].on_prep = partner->UsingPrEP();
            G[partner_vertex].on_ART = partner->isOnArt();
            G[partner_vertex].risk_level = (int)partner->getRiskLevel();
        } else {
            partner_vertex = *vertex_iter;
        }

        assert(entity_vertex != partner_vertex);
        boost::tie(partnership_edge, inserted) = add_edge(entity_vertex,
            partner_vertex, G);
        G[partnership_edge].type = (int)partnership->getType();
    }

    void Write(int month)
    {
        boost::dynamic_properties dp;
        dp.property("id", get(&EntityVertex::id, G));
        dp.property("hiv_pos", get(&EntityVertex::hiv_pos, G));
        dp.property("on_prep", get(&EntityVertex::on_prep, G));
        dp.property("on_ART", get(&EntityVertex::on_ART, G));
        dp.property("risk_level", get(&EntityVertex::risk_level, G));

        dp.property("type", get(&PartnershipEdge::type, G));

        std::string filename = "PartnershipNetwork_" + std::to_string(month) + ".graphml";
        std::ofstream ofs(filename);
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

private:
    Graph G;

};
