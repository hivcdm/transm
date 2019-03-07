#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphml.hpp>

#include "entities/entity.hpp"
#include "entities/demographicprofile.hpp"
#include "entities/sexualpartnership.hpp"
#include "utility/time.hpp"

/**
   This class outputs the partnership network in graphml format
 **/

class Network {

    struct EntityVertex
    {
        int id;;
        std::string gender;

        bool is_alive = true;
        int time_of_SA = 0;
        int time_of_death = 1020;

        bool is_CSW;
        bool hiv_pos;
        int time_infected;
        bool on_prep;
        bool on_ART;
        bool risk_level;
        int viral_load;
        int composite; // on_prep=-1,hiv_neg=0,hiv_pos=viral_load
    };

    struct PartnershipEdge
    {
        int type;
        int start;
        int end;
        int duration;
        std::string gender_to_gender; //e.g msw+female,msm+msm,etc
        int sero_pos; // both_hiv-=0,one_hiv+=1,both_hiv+=2
    };

    // Adjacency List
    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                                  EntityVertex, PartnershipEdge> Graph;
    typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;
    typedef boost::graph_traits<Graph>::vertex_iterator vertex_iter_t;
    typedef boost::graph_traits<Graph>::edge_descriptor edge_t;
    typedef boost::graph_traits<Graph>::edge_iterator edge_iter_t;

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

    void SetVertexAttributes(vertex_t entity_vertex, transm::Entity *entity, bool inserted, int month)
    {
        if (inserted)
        {
            G[entity_vertex].time_of_SA = month;
        }

        if (!entity->isAlive())
        {
            G[entity_vertex].time_of_death = month;
        }

        bool hiv_pos = (entity->getHIVStatus() != transm::Entity::HIVStatus::NEGATIVE);
        bool on_PrEP = entity->UsingPrEP();
        bool on_ART = entity->isOnArt();
        int viral_load = hiv_pos ? (int)entity->getHvlStratum() : 0;

        G[entity_vertex].id = entity->getID();
        G[entity_vertex].gender = entity->getEntityType();
        G[entity_vertex].is_CSW = entity->isCSW();
        G[entity_vertex].risk_level = (bool)entity->getRiskLevel();
        G[entity_vertex].on_prep = on_PrEP;
        G[entity_vertex].on_ART = on_ART;
        G[entity_vertex].viral_load = viral_load;
        G[entity_vertex].composite = (!on_PrEP) ? viral_load : -1;

        if (!G[entity_vertex].hiv_pos)
        {
            G[entity_vertex].hiv_pos = hiv_pos;
            G[entity_vertex].time_infected = month;
        }
    }

    void SetEdgeAttributes(edge_t partnership_edge, transm::Entity *entity, transm::Entity *partner,
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

    void AddPartnership(transm::Entity *entity, transm::Entity *partner,
        transm::SexualPartnership *partnership, int month)
    {
        vertex_iter_t entity_itr, partner_itr;
        vertex_t entity_vertex, partner_vertex;
        edge_t partnership_edge;
        bool inserted;

        entity_itr = FindVertex(G, entity->getID());
        if (entity_itr == boost::vertices(G).second)
        {
            entity_vertex = add_vertex(G);
            SetVertexAttributes(entity_vertex, entity, true, month);
        } else {
            entity_vertex = *entity_itr;
        }

        partner_itr = FindVertex(G, partner->getID());
        if (partner_itr == boost::vertices(G).second)
        {
            partner_vertex = add_vertex(G);
            SetVertexAttributes(partner_vertex, partner, true, month);
        } else {
            partner_vertex = *partner_itr;
        }

        assert(entity_vertex != partner_vertex);
        boost::tie(partnership_edge, inserted) = add_edge(entity_vertex,
            partner_vertex, G);
        SetEdgeAttributes(partnership_edge, entity, partner, partnership);
    }

    void UpdateEntity(transm::Entity *entity, int month)
    {
        vertex_iter_t entity_itr;
        vertex_t entity_vertex;

        entity_itr = FindVertex(G, entity->getID());
        if (entity_itr == boost::vertices(G).second)
        {
            return;
        }
        entity_vertex = *entity_itr;
        SetVertexAttributes(entity_vertex, entity, false, month);
    }

    void Write(int month)
    {
        boost::dynamic_properties dp;

        // node properties
        dp.property("id", get(&EntityVertex::id, G));
        dp.property("gender", get(&EntityVertex::gender, G));
        dp.property("time_SA", get(&EntityVertex::time_of_SA, G));
        dp.property("time_death", get(&EntityVertex::time_of_death, G));
        dp.property("is_CSW", get(&EntityVertex::is_CSW, G));
        dp.property("hiv_pos", get(&EntityVertex::hiv_pos, G));
        dp.property("time_infected", get(&EntityVertex::time_infected, G));
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
    }

private:
    Graph G;

};
