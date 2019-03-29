#pragma once

#include "nifty/graph/rag/grid_rag.hxx"
#include "nifty/xtensor/xtensor.hxx"

#include "nifty/tools/for_each_coordinate.hxx"

namespace nifty{
namespace graph{

    // TODO parallelize
    template<std::size_t DIM, class GRAPH_LABELS, class LABELS, class NODE_MAP>
    void gridRagAccumulateLabels(const GridRag<DIM, GRAPH_LABELS> & graph,
                                 const xt::xexpression<LABELS> & dataExp,
                                 NODE_MAP & nodeMap,
                                 const bool ignoreBackground = false,
                                 const typename LABELS::value_type ignoreValue = 0){
        typedef std::array<int64_t, DIM> Coord;
        typedef typename LABELS::value_type LabelType;

        const auto & labels = graph.labels();
        const auto & shape = graph.shape();
        const auto & data = dataExp.derived_cast();

        std::vector<std::unordered_map<LabelType, std::size_t>> overlaps(graph.numberOfNodes());

        nifty::tools::forEachCoordinate(shape, [&](const Coord & coord){
            const auto node = xtensor::read(labels, coord);
            const auto l = xtensor::read(data, coord);
            overlaps[node][l] += 1;
        });

        for(const auto node : graph.nodes()){
            const auto & ol = overlaps[node];
            // find max ol
            std::size_t maxOl = 0;
            LabelType maxOlLabel = 0;
            for(auto kv : ol){
                if(kv.second > maxOl){
                    if(ignoreBackground && kv.first == ignoreValue) {
                        continue;
                    }
                    maxOl = kv.second;
                    maxOlLabel = kv.first;
                }
            }
            nodeMap[node] = maxOlLabel;
        }
    }


    template<std::size_t DIM, class GRAPH_LABELS, class NODE_MAP>
    void findZExtendedNodes(const GridRag<DIM, GRAPH_LABELS> & graph, NODE_MAP & extendedNodes){
        typedef std::array<int64_t, DIM> Coord;
        typedef typename GridRag<DIM, GRAPH_LABELS>::value_type LabelType;

        const auto & labels = graph.labels();
        const auto & shape = graph.shape();

        std::vector<std::set<std::size_t>> zCoordinates(graph.numberOfNodes());

        nifty::tools::forEachCoordinate(shape, [&](const Coord & coord){
            const auto node = xtensor::read(labels, coord);
            const std::size_t z = coord[0];
            zCoordinates[node].insert(z);
        });

        for(LabelType nodeId = 0; nodeId < graph.numberOfNodes(); ++nodeId) {
            if(zCoordinates[nodeId].size() > 1) {
                extendedNodes.push_back(nodeId);
            }
        }
    }


} // end namespace graph
} // end namespace nifty
