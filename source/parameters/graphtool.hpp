#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphml.hpp>
//#include <graph.hh>

#include "entities/entity.hpp"

//using namespace graph_tool;

/**
   This is the interface with graph_tool https://graph-tool.skewed.de/
 **/

namespace transm {

class Network
{
public:

    typedef boost::property<boost::edge_weight_t, int> EdgeWeightProperty;
    typedef boost::adjacency_list<boost::listS, boost::vecS,boost::undirectedS,
                                  boost::no_property,EdgeWeightProperty> Graph;
    Network() {}

    void UpdatePartnership(unsigned long entityId, unsigned long partnerId,
        int type)
    {
        boost::add_edge(entityId, partnerId, type, G);
    }

    void Write()
    {
        boost::dynamic_properties dp;
        //dp.property("type", get(Graph::EdgeWeightProperty(), G));

        std::ofstream ofs("ParntershipNetwork.gml");
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

} //namespace transm
