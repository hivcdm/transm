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
        std::string gender;
        bool is_CSW;
        bool hiv_pos;
        bool on_prep;
        bool on_ART;
        bool risk_level;
        int viral_load;
        int composite; // on_prep=-1,hiv_neg=0,hiv_pos=viral_load
        //boost::vertex_orientation_t orientation;
    };

    struct PartnershipEdge
    {
        int type;
        int start;
        int end;
        int duration;
        std::string gender_to_gender; //e.g msw+female,msm+msm,etc
        int sero_pos; // both_hiv-=0,one_hiv+=1,both_hiv+=2
        //boost::edge_type_t type;
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

    void AddEntity(vertex_t entity_vertex, transm::Entity *entity)
    {
        bool hiv_pos = (entity->getHIVStatus() != transm::Entity::HIVStatus::NEGATIVE);
        bool on_PrEP = entity->UsingPrEP();
        bool on_ART = entity->isOnArt();
        int viral_load = hiv_pos ? (int)entity->getHvlStratum() : 0;

        G[entity_vertex].id = entity->getID();
        G[entity_vertex].gender = entity->getEntityType();
        G[entity_vertex].is_CSW = entity->isCSW();
        G[entity_vertex].risk_level = (bool)entity->getRiskLevel();
        G[entity_vertex].hiv_pos = hiv_pos;
        G[entity_vertex].on_prep = on_PrEP;
        G[entity_vertex].on_ART = on_ART;
        G[entity_vertex].viral_load = viral_load;
        G[entity_vertex].composite = (!on_PrEP) ? viral_load : -1;
    }

    void AddPartnership(edge_t partnership_edge, transm::Entity *entity, transm::Entity *partner,
        transm::SexualPartnership *partnership)
    {
        std::string entity_gender =  entity->getEntityType();
        std::string partner_gender = partner->getEntityType();

        bool entity_hiv_pos = (entity->getHIVStatus() != transm::Entity::HIVStatus::NEGATIVE);
        bool partner_hiv_pos = (partner->getHIVStatus() != transm::Entity::HIVStatus::NEGATIVE);

        int time_of_formation = (int)partnership->getTimeOfFormation().in_months();
        int time_of_dissolution = (int)partnership->getTimeOfDissolution().in_months();

        G[partnership_edge].type = (int)partnership->getType();
        G[partnership_edge].start = time_of_formation;
        G[partnership_edge].end = time_of_dissolution;
        G[partnership_edge].duration = time_of_dissolution - time_of_formation;
        G[partnership_edge].gender_to_gender = entity_gender + "+" + partner_gender;
        G[partnership_edge].sero_pos = (int)entity_hiv_pos + (int)partner_hiv_pos;
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
            AddEntity(entity_vertex, entity);
        } else {
            entity_vertex = *vertex_iter;
        }

        vertex_iter = FindVertex(G, partner->getID());
        if (vertex_iter == boost::vertices(G).second)
        {
            partner_vertex = add_vertex(G);
            AddEntity(partner_vertex, partner);
        } else {
            partner_vertex = *vertex_iter;
        }

        assert(entity_vertex != partner_vertex);
        boost::tie(partnership_edge, inserted) = add_edge(entity_vertex,
            partner_vertex, G);
        AddPartnership(partnership_edge, entity, partner, partnership);
    }

    void Write(int month)
    {
        boost::dynamic_properties dp;

        // node properties
        dp.property("id", get(&EntityVertex::id, G));
        dp.property("gender", get(&EntityVertex::gender, G));
        dp.property("is_CSW", get(&EntityVertex::is_CSW, G));
        dp.property("hiv_pos", get(&EntityVertex::hiv_pos, G));
        dp.property("on_prep", get(&EntityVertex::on_prep, G));
        dp.property("on_ART", get(&EntityVertex::on_ART, G));
        dp.property("risk_level", get(&EntityVertex::risk_level, G));
        dp.property("viral_load", get(&EntityVertex::viral_load, G));
        dp.property("composite", get(&EntityVertex::composite, G));

        // edge properties
        dp.property("type", get(&PartnershipEdge::type, G));
        dp.property("start", get(&PartnershipEdge::start, G));
        dp.property("end", get(&PartnershipEdge::end, G));
        dp.property("duration", get(&PartnershipEdge::duration, G));
        dp.property("genders", get(&PartnershipEdge::gender_to_gender, G));
        dp.property("sero_pos", get(&PartnershipEdge::sero_pos, G));

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
