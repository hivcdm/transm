#ifndef PARTNERNETWORK_HPP
#define PARTNERNETWORK_HPP

#include <fstream>
#include <set>
#include <string>
#include <unordered_map>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphml.hpp>

#include "entities/entitytypes.hpp"
#include "entities/entity.hpp"
#include "entities/demographicprofile.hpp"
#include "entities/focusgroup.hpp"
#include "entities/sexualpartnership.hpp"
#include "utility/time.hpp"

/** This class outputs the partnership network in graphml format */
class Network {

    struct EntityVertex
    {
        int id;
        int gender;
        int orientation;
        int race;
        int ethnicity;
        std::string race_eth;
        std::string demo_profile;
        bool is_CSW;
        bool hiv_pos;
        bool on_prep;
        bool on_ART;
        bool risk_level;
        int viral_load;
        int hiv_composite; /* on_prep=-1,hiv_neg=0,hiv_pos=viral_load */

        /* Care continuum. FOCUS acts by detecting and re-linking, so these are
         * what make a FOCUS flag legible: they show what the selection did. */
        bool detected;
        bool linked;
        bool in_care;
        bool ltfu;
        bool rtc;
        int care_state; /* SimContext::HIV_CARE_*, -1 without a CEPAC patient */

        /* FOCUS module */
        bool focus_selected;      /* ever selected by FOCUS */
        bool focus_screened;      /* ever sampled by FOCUS, selected or not */
        int focus_month;          /* month of first selection, -1 if never */
        int focus_month_last;     /* month of most recent selection, -1 if never */
        int months_since_focus;   /* age of the most recent selection, -1 if never */
        int focus_group;          /* FocusGroup of the selection, or of the last
                                   * screening if never selected; -1 if neither */
        std::string focus_group_str;  /* e.g. "BLACK:MALE:LTFU", "NONE" if never */
        int focus_reason;         /* FocusReason: 0=undiagnosed,1=LTFU,-1=none */
        int focus_select_count;   /* times selected */
        int focus_screen_count;   /* times sampled */
        int focus_status;         /* FocusStatus composite, see focusgroup.hpp */

        /* Structural */
        bool is_isolate; /* no partnerships in this snapshot */
    };

    struct PartnershipEdge
    {
        int type;
        int start;
        int end;
        int duration;
        std::string gender_to_gender; /* e.g msw+female,msm+msm,etc */
        int sero_pos; /* both_hiv-=0,one_hiv+=1,both_hiv+=2 */
        int focus_edge; /* endpoints ever selected by FOCUS: 0, 1 or 2 */
    };

    /* Adjacency List */
    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                                  EntityVertex, PartnershipEdge> Graph;
    typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;
    typedef boost::graph_traits<Graph>::vertex_iterator vertex_iter_t;
    typedef boost::graph_traits<Graph>::edge_descriptor edge_t;

    void AddEntity(vertex_t entity_vertex, transm::Entity *entity)
    {
        auto gender = entity->getDemographicProfileVal<transm::DemographicProfile::Gender>();
        auto orientation = entity->getDemographicProfileVal<transm::DemographicProfile::SexualOrientation>();
        auto race = entity->getDemographicProfileVal<transm::DemographicProfile::Race>();
        auto ethnicity = entity->getDemographicProfileVal<transm::DemographicProfile::Ethnicity>();

        std::string demo_profile = entity->getEntityType();
        std::string race_eth = ":" + transm::DemographicEnumStrs.at((std::size_t)transm::DemographicProfile::Demographic::Race).at((std::size_t)race) + ":" + transm::DemographicEnumStrs.at((std::size_t)transm::DemographicProfile::Demographic::Ethnicity).at((std::size_t)ethnicity);
        demo_profile += race_eth;

        bool hiv_pos = (entity->getHIVStatus() != transm::HIVStatus::NEGATIVE);
        bool on_PrEP = entity->UsingPrEP();
        bool on_ART = entity->isOnArt();
        int viral_load = hiv_pos ? (int)entity->getHvlStratum() : 0;

        int focus_group = entity->getFOCUSGroup();

        G[entity_vertex].id = entity->getID();
        G[entity_vertex].gender = (int)gender;
        G[entity_vertex].orientation = (int)orientation;
        G[entity_vertex].race = (int)race;
        G[entity_vertex].ethnicity = (int)ethnicity;
        G[entity_vertex].demo_profile = demo_profile;
        G[entity_vertex].race_eth = race_eth;
        G[entity_vertex].is_CSW = entity->isCSW();
        G[entity_vertex].risk_level = (bool)entity->getRiskLevel();
        G[entity_vertex].hiv_pos = hiv_pos;
        G[entity_vertex].on_prep = on_PrEP;
        G[entity_vertex].on_ART = on_ART;
        G[entity_vertex].viral_load = viral_load;
        G[entity_vertex].hiv_composite = (!on_PrEP) ? viral_load : -1;

        G[entity_vertex].detected = entity->isDetected();
        G[entity_vertex].linked = entity->isLinked();
        G[entity_vertex].in_care = entity->isInCare();
        G[entity_vertex].ltfu = entity->isLTFU();
        G[entity_vertex].rtc = entity->isRTC();
        G[entity_vertex].care_state = entity->getCareState();

        G[entity_vertex].focus_selected = entity->wasSelectedForFOCUS();
        G[entity_vertex].focus_screened = entity->wasScreenedByFOCUS();
        G[entity_vertex].focus_month = entity->getFOCUSMonthFirstSelected();
        G[entity_vertex].focus_month_last = entity->getFOCUSMonthLastSelected();
        G[entity_vertex].months_since_focus = entity->getMonthsSinceFOCUS(month_);
        G[entity_vertex].focus_group = focus_group;
        G[entity_vertex].focus_group_str = transm::focusGroupStr(focus_group);
        G[entity_vertex].focus_reason = transm::focusGroupReason(focus_group);
        G[entity_vertex].focus_select_count = entity->getFOCUSSelectCount();
        G[entity_vertex].focus_screen_count = entity->getFOCUSScreenCount();
        G[entity_vertex].focus_status = entity->getFOCUSStatus(month_);

        /* Corrected in Write() once every edge is in place */
        G[entity_vertex].is_isolate = true;
    }

    void AddPartnership(edge_t partnership_edge, transm::Entity *entity, transm::Entity *partner,
        transm::SexualPartnership *partnership)
    {
        std::string entity_gender =  entity->getEntityType();
        std::string partner_gender = partner->getEntityType();

        bool entity_hiv_pos = (entity->getHIVStatus() != transm::HIVStatus::NEGATIVE);
        bool partner_hiv_pos = (partner->getHIVStatus() != transm::HIVStatus::NEGATIVE);

        int time_of_formation = (int)partnership->getTimeOfFormation().in_months();
        int time_of_dissolution = (int)partnership->getTimeOfDissolution().in_months();

        G[partnership_edge].type = (int)partnership->getType();
        G[partnership_edge].start = time_of_formation;
        G[partnership_edge].end = time_of_dissolution;
        G[partnership_edge].duration = time_of_dissolution - time_of_formation;
        G[partnership_edge].gender_to_gender = entity_gender + "+" + partner_gender;
        G[partnership_edge].sero_pos = (int)entity_hiv_pos + (int)partner_hiv_pos;
        /* Same shape as sero_pos, so the two can be crossed to ask whether
         * FOCUS is reaching the serodiscordant edges */
        G[partnership_edge].focus_edge = (int)entity->wasSelectedForFOCUS() +
                                         (int)partner->wasSelectedForFOCUS();
    }

    /** Find this entity's vertex, creating and populating it on first sight.
     * The id -> vertex map keeps this O(1); it used to be a linear scan over
     * every vertex, which made building the graph O(V*E). */
    vertex_t EnsureVertex(transm::Entity *entity)
    {
        const int id = (int)entity->getID();

        auto it = vertex_index_.find(id);
        if (it != vertex_index_.end())
            return it->second;

        vertex_t entity_vertex = add_vertex(G);
        AddEntity(entity_vertex, entity);
        vertex_index_[id] = entity_vertex;
        return entity_vertex;
    }

public:
    /** @param month the simulation month this snapshot is taken in; used both
     * for the filename and to age the per-entity FOCUS record */
    explicit Network(int month) : month_(month) {}

    /** Put an entity in the graph even if it has no partnerships.
     *
     * FOCUS targets undiagnosed and lost-to-follow-up people, a good share of
     * whom have no active partnership in any given month. Building the graph
     * from partnerships alone would drop exactly those people, so the module
     * would look like it had less reach than it does. */
    void AddIsolatedEntity(transm::Entity *entity)
    {
        EnsureVertex(entity);
    }

    void UpdatePartnership(transm::Entity *entity, transm::Entity *partner,
        transm::SexualPartnership *partnership)
    {
        /* Population::WritePartnershipNetwork walks every entity's partnership
         * list, so each partnership is offered to us twice, once from either
         * endpoint. Keying on the partnership itself - rather than on the
         * (entity, partner) pair - drops the duplicate without also dropping a
         * genuine second partnership between the same two people. */
        if (!seen_partnerships_.insert(partnership).second)
            return;

        vertex_t entity_vertex = EnsureVertex(entity);
        vertex_t partner_vertex = EnsureVertex(partner);

        assert(entity_vertex != partner_vertex);

        edge_t partnership_edge;
        bool inserted;
        boost::tie(partnership_edge, inserted) = add_edge(entity_vertex,
            partner_vertex, G);
        AddPartnership(partnership_edge, entity, partner, partnership);
    }

    void Write()
    {
        /* Now that every edge is in, mark the entities with no partnerships so
         * they can be filtered back out in Gephi if they are in the way */
        vertex_iter_t vi, vi_end;
        for (boost::tie(vi, vi_end) = boost::vertices(G); vi != vi_end; ++vi)
            G[*vi].is_isolate = (boost::degree(*vi, G) == 0);

        boost::dynamic_properties dp;

        /* node properties */
        dp.property("id", get(&EntityVertex::id, G));
        dp.property("gender", get(&EntityVertex::gender, G));
        dp.property("orientation", get(&EntityVertex::orientation, G));
        dp.property("race", get(&EntityVertex::race, G));
        dp.property("ethnicity", get(&EntityVertex::ethnicity, G));
        dp.property("race_eth", get(&EntityVertex::race_eth, G));
        dp.property("demo_profile", get(&EntityVertex::demo_profile, G));
        dp.property("is_CSW", get(&EntityVertex::is_CSW, G));
        dp.property("hiv_pos", get(&EntityVertex::hiv_pos, G));
        dp.property("on_prep", get(&EntityVertex::on_prep, G));
        dp.property("on_ART", get(&EntityVertex::on_ART, G));
        dp.property("risk_level", get(&EntityVertex::risk_level, G));
        dp.property("viral_load", get(&EntityVertex::viral_load, G));
        dp.property("hiv_composite", get(&EntityVertex::hiv_composite, G));

        /* node properties - care continuum */
        dp.property("detected", get(&EntityVertex::detected, G));
        dp.property("linked", get(&EntityVertex::linked, G));
        dp.property("in_care", get(&EntityVertex::in_care, G));
        dp.property("ltfu", get(&EntityVertex::ltfu, G));
        dp.property("rtc", get(&EntityVertex::rtc, G));
        dp.property("care_state", get(&EntityVertex::care_state, G));

        /* node properties - FOCUS */
        dp.property("focus_selected", get(&EntityVertex::focus_selected, G));
        dp.property("focus_screened", get(&EntityVertex::focus_screened, G));
        dp.property("focus_month", get(&EntityVertex::focus_month, G));
        dp.property("focus_month_last", get(&EntityVertex::focus_month_last, G));
        dp.property("months_since_focus", get(&EntityVertex::months_since_focus, G));
        dp.property("focus_group", get(&EntityVertex::focus_group, G));
        dp.property("focus_group_str", get(&EntityVertex::focus_group_str, G));
        dp.property("focus_reason", get(&EntityVertex::focus_reason, G));
        dp.property("focus_select_count", get(&EntityVertex::focus_select_count, G));
        dp.property("focus_screen_count", get(&EntityVertex::focus_screen_count, G));
        dp.property("focus_status", get(&EntityVertex::focus_status, G));

        /* node properties - structural */
        dp.property("is_isolate", get(&EntityVertex::is_isolate, G));

        /* edge properties */
        dp.property("type", get(&PartnershipEdge::type, G));
        dp.property("start", get(&PartnershipEdge::start, G));
        dp.property("end", get(&PartnershipEdge::end, G));
        dp.property("duration", get(&PartnershipEdge::duration, G));
        dp.property("genders", get(&PartnershipEdge::gender_to_gender, G));
        dp.property("sero_pos", get(&PartnershipEdge::sero_pos, G));
        dp.property("focus_edge", get(&PartnershipEdge::focus_edge, G));

        std::string filename = prefix_ + "PartnershipNetwork_" + std::to_string(month_) + ".graphml";
        std::ofstream ofs(filename);
        boost::write_graphml(ofs, G, dp, true);
    }

    /** Prepend a run identifier to the output filename. Without one, every run
     * in a batch writes the same PartnershipNetwork_<month>.graphml into the
     * shared results directory and clobbers the others. */
    void SetFilenamePrefix(const std::string &prefix) { prefix_ = prefix; }

private:
    Graph G;

    /** simulation month of this snapshot */
    int month_;

    /** run identifier prepended to the output filename, may be empty */
    std::string prefix_;

    /** entity id -> vertex, so lookup does not scan the whole graph */
    std::unordered_map<int, vertex_t> vertex_index_;

    /** partnerships already added, so the two-sided walk cannot double them */
    std::set<const transm::SexualPartnership *> seen_partnerships_;

};

#endif /* PARTNERNETWORK_HPP */
