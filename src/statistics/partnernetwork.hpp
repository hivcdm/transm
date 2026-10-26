#ifndef PARTNERNETWORK_HPP
#define PARTNERNETWORK_HPP

#include <fstream>
#include <set>
#include <string>
#include <unordered_map>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphml.hpp>
#include <boost/property_map/function_property_map.hpp>

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

    struct EntityReference
    {
        transm::Entity *entity;
    };

    struct PartnershipReference
    {
        transm::Entity *entity;
        transm::Entity *partner;
        transm::SexualPartnership *partnership;
    };

    /* Keep topology and references only. Attributes are read while writing,
     * while the population and partnerships are still alive and unchanged. */
    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                                  EntityReference, PartnershipReference> Graph;
    typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;
    typedef boost::graph_traits<Graph>::vertex_iterator vertex_iter_t;
    typedef boost::graph_traits<Graph>::edge_descriptor edge_t;

    EntityVertex ReadEntity(vertex_t vertex) const
    {
        transm::Entity *entity = G[vertex].entity;
        EntityVertex attributes;
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

        attributes.id = entity->getID();
        attributes.gender = (int)gender;
        attributes.orientation = (int)orientation;
        attributes.race = (int)race;
        attributes.ethnicity = (int)ethnicity;
        attributes.demo_profile = demo_profile;
        attributes.race_eth = race_eth;
        attributes.is_CSW = entity->isCSW();
        attributes.risk_level = (bool)entity->getRiskLevel();
        attributes.hiv_pos = hiv_pos;
        attributes.on_prep = on_PrEP;
        attributes.on_ART = on_ART;
        attributes.viral_load = viral_load;
        attributes.hiv_composite = (!on_PrEP) ? viral_load : -1;

        attributes.detected = entity->isDetected();
        attributes.linked = entity->isLinked();
        attributes.in_care = entity->isInCare();
        attributes.ltfu = entity->isLTFU();
        attributes.rtc = entity->isRTC();
        attributes.care_state = entity->getCareState();

        attributes.focus_selected = entity->wasSelectedForFOCUS();
        attributes.focus_screened = entity->wasScreenedByFOCUS();
        attributes.focus_month = entity->getFOCUSMonthFirstSelected();
        attributes.focus_month_last = entity->getFOCUSMonthLastSelected();
        attributes.months_since_focus = entity->getMonthsSinceFOCUS(month_);
        attributes.focus_group = focus_group;
        attributes.focus_group_str = transm::focusGroupStr(focus_group);
        attributes.focus_reason = transm::focusGroupReason(focus_group);
        attributes.focus_select_count = entity->getFOCUSSelectCount();
        attributes.focus_screen_count = entity->getFOCUSScreenCount();
        attributes.focus_status = entity->getFOCUSStatus(month_);

        attributes.is_isolate = (boost::degree(vertex, G) == 0);
        return attributes;
    }

    PartnershipEdge ReadPartnership(edge_t edge) const
    {
        const auto &reference = G[edge];
        auto *entity = reference.entity;
        auto *partner = reference.partner;
        auto *partnership = reference.partnership;
        PartnershipEdge attributes;
        std::string entity_gender =  entity->getEntityType();
        std::string partner_gender = partner->getEntityType();

        bool entity_hiv_pos = (entity->getHIVStatus() != transm::HIVStatus::NEGATIVE);
        bool partner_hiv_pos = (partner->getHIVStatus() != transm::HIVStatus::NEGATIVE);

        int time_of_formation = (int)partnership->getTimeOfFormation().in_months();
        int time_of_dissolution = (int)partnership->getTimeOfDissolution().in_months();

        attributes.type = (int)partnership->getType();
        attributes.start = time_of_formation;
        attributes.end = time_of_dissolution;
        attributes.duration = time_of_dissolution - time_of_formation;
        attributes.gender_to_gender = entity_gender + "+" + partner_gender;
        attributes.sero_pos = (int)entity_hiv_pos + (int)partner_hiv_pos;
        /* Same shape as sero_pos, so the two can be crossed to ask whether
         * FOCUS is reaching the serodiscordant edges */
        attributes.focus_edge = (int)entity->wasSelectedForFOCUS() +
                                         (int)partner->wasSelectedForFOCUS();
        return attributes;
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
        G[entity_vertex].entity = entity;
        vertex_index_[id] = entity_vertex;
        return entity_vertex;
    }

    template<typename Value>
    auto VertexProperty(Value EntityVertex::*member)
    {
        return boost::make_function_property_map<vertex_t>([this, member](vertex_t vertex) {
            if (!have_vertex_attributes_ || vertex != cached_vertex_) {
                vertex_attributes_ = ReadEntity(vertex);
                cached_vertex_ = vertex;
                have_vertex_attributes_ = true;
            }
            return vertex_attributes_.*member;
        });
    }

    template<typename Value>
    auto EdgeProperty(Value PartnershipEdge::*member)
    {
        return boost::make_function_property_map<edge_t>([this, member](edge_t edge) {
            if (!have_edge_attributes_ || edge != cached_edge_) {
                edge_attributes_ = ReadPartnership(edge);
                cached_edge_ = edge;
                have_edge_attributes_ = true;
            }
            return edge_attributes_.*member;
        });
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
        G[partnership_edge] = {entity, partner, partnership};
    }

    void Write()
    {
        /* Boost visits all properties of one row together. Cache just that
         * row so strings are formatted once, without retaining a second copy
         * of the population's attributes for the whole export. */
        have_vertex_attributes_ = false;
        have_edge_attributes_ = false;

        boost::dynamic_properties dp;

        /* node properties */
        dp.property("id", VertexProperty(&EntityVertex::id));
        dp.property("gender", VertexProperty(&EntityVertex::gender));
        dp.property("orientation", VertexProperty(&EntityVertex::orientation));
        dp.property("race", VertexProperty(&EntityVertex::race));
        dp.property("ethnicity", VertexProperty(&EntityVertex::ethnicity));
        dp.property("race_eth", VertexProperty(&EntityVertex::race_eth));
        dp.property("demo_profile", VertexProperty(&EntityVertex::demo_profile));
        dp.property("is_CSW", VertexProperty(&EntityVertex::is_CSW));
        dp.property("hiv_pos", VertexProperty(&EntityVertex::hiv_pos));
        dp.property("on_prep", VertexProperty(&EntityVertex::on_prep));
        dp.property("on_ART", VertexProperty(&EntityVertex::on_ART));
        dp.property("risk_level", VertexProperty(&EntityVertex::risk_level));
        dp.property("viral_load", VertexProperty(&EntityVertex::viral_load));
        dp.property("hiv_composite", VertexProperty(&EntityVertex::hiv_composite));

        /* node properties - care continuum */
        dp.property("detected", VertexProperty(&EntityVertex::detected));
        dp.property("linked", VertexProperty(&EntityVertex::linked));
        dp.property("in_care", VertexProperty(&EntityVertex::in_care));
        dp.property("ltfu", VertexProperty(&EntityVertex::ltfu));
        dp.property("rtc", VertexProperty(&EntityVertex::rtc));
        dp.property("care_state", VertexProperty(&EntityVertex::care_state));

        /* node properties - FOCUS */
        dp.property("focus_selected", VertexProperty(&EntityVertex::focus_selected));
        dp.property("focus_screened", VertexProperty(&EntityVertex::focus_screened));
        dp.property("focus_month", VertexProperty(&EntityVertex::focus_month));
        dp.property("focus_month_last", VertexProperty(&EntityVertex::focus_month_last));
        dp.property("months_since_focus", VertexProperty(&EntityVertex::months_since_focus));
        dp.property("focus_group", VertexProperty(&EntityVertex::focus_group));
        dp.property("focus_group_str", VertexProperty(&EntityVertex::focus_group_str));
        dp.property("focus_reason", VertexProperty(&EntityVertex::focus_reason));
        dp.property("focus_select_count", VertexProperty(&EntityVertex::focus_select_count));
        dp.property("focus_screen_count", VertexProperty(&EntityVertex::focus_screen_count));
        dp.property("focus_status", VertexProperty(&EntityVertex::focus_status));

        /* node properties - structural */
        dp.property("is_isolate", VertexProperty(&EntityVertex::is_isolate));

        /* edge properties */
        dp.property("type", EdgeProperty(&PartnershipEdge::type));
        dp.property("start", EdgeProperty(&PartnershipEdge::start));
        dp.property("end", EdgeProperty(&PartnershipEdge::end));
        dp.property("duration", EdgeProperty(&PartnershipEdge::duration));
        dp.property("genders", EdgeProperty(&PartnershipEdge::gender_to_gender));
        dp.property("sero_pos", EdgeProperty(&PartnershipEdge::sero_pos));
        dp.property("focus_edge", EdgeProperty(&PartnershipEdge::focus_edge));

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
    EntityVertex vertex_attributes_;
    PartnershipEdge edge_attributes_;
    vertex_t cached_vertex_{};
    edge_t cached_edge_{};
    bool have_vertex_attributes_ = false;
    bool have_edge_attributes_ = false;

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
