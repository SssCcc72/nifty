#pragma once

#include "nifty/parallel/threadpool.hxx"
#include "nifty/tools/array_tools.hxx"
#include "nifty/graph/undirected_list_graph.hxx"
#include "nifty/tools/block_access.hxx"
#include "nifty/xtensor/xtensor.hxx"

namespace nifty {
namespace graph {


template<class LABELS>
class LongRangeAdjacency : public UndirectedGraph<>{

public:
    typedef LABELS Labels;
    typedef typename Labels::value_type LabelType;
    typedef UndirectedGraph<> BaseType;

    typedef array::StaticArray<int64_t, 3> Coord;
    typedef array::StaticArray<int64_t, 2> Coord2;

    // constructor from data
    LongRangeAdjacency(
        const Labels & labels,
        const size_t range,
        const size_t numberOfLabels,
        const bool ignoreLabel,
        const int numberOfThreads=-1
    ) : range_(range),
        shape_({labels.shape()[0], labels.shape()[1], labels.shape()[2]}),
        numberOfEdgesInSlice_(shape_[0]),
        edgeOffset_(shape_[0]),
        ignoreLabel_(ignoreLabel)
    {
        initAdjacency(labels, numberOfLabels, numberOfThreads);
    }

    // constructor from serialization
    template<class ITER>
    LongRangeAdjacency(
        const Labels & labels,
        ITER & iter
    ) : range_(0),
        shape_({labels.shape()[0], labels.shape()[1], labels.shape()[2]}),
        numberOfEdgesInSlice_(shape_[0]),
        edgeOffset_(shape_[0])
    {
        deserializeAdjacency(iter);
    }

    // API

    std::size_t range() const {
        return range_;
    }

    std::size_t numberOfEdgesInSlice(const std::size_t z) const {
        return numberOfEdgesInSlice_[z];
    }

    std::size_t edgeOffset(const std::size_t z) const {
        return edgeOffset_[z];
    }

    std::size_t serializationSize() const {
        std::size_t size = BaseType::serializationSize();
        size += 2; // increase by 2 for the fields longRange and ignoreLabel
        size += shape_[0] * 2; // increase by slice vector sizes
        return size;
    }

    int64_t shape(const std::size_t i) const {
        return shape_[i];
    }

    const Coord & shape() const {
        return shape_;
    }

    template<class ITER>
    void serialize(ITER & iter) const {
        *iter = range_;
        ++iter;
        *iter = ignoreLabel_ ? 1 : 0;
        ++iter;
        std::size_t nSlices = shape_[0];
        for(std::size_t slice = 0; slice < nSlices; ++slice) {
            *iter = numberOfEdgesInSlice_[slice];
            ++iter;
            *iter = edgeOffset_[slice];
            ++iter;
        }
        BaseType::serialize(iter);
    }

    bool hasIgnoreLabel() const {
        return ignoreLabel_;
    }

private:
    void initAdjacency(const Labels & labels, const std::size_t numberOfLabels, const int numberOfThreads);

    template<class ITER>
    void deserializeAdjacency(ITER & iter) {
        range_ = *iter;
        ++iter;
        ignoreLabel_ = (*iter == 1) ? true : false;
        ++iter;
        std::size_t nSlices = shape_[0];
        for(std::size_t slice = 0; slice < nSlices; ++slice) {
            numberOfEdgesInSlice_[slice] = *iter;
            ++iter;
            edgeOffset_[slice] = *iter;
            ++iter;
        }
        BaseType::deserialize(iter);
    }

    Coord shape_;
    std::size_t range_;
    std::vector<std::size_t> numberOfEdgesInSlice_;
    std::vector<std::size_t> edgeOffset_;
    bool ignoreLabel_;
};


// FIXME multithreading sometimes causes segfaults / undefined behaviour, if we have an ignore label
template<class LABELS>
void LongRangeAdjacency<LABELS>::initAdjacency(const LABELS & labels,
                                               const std::size_t numberOfLabels,
                                               const int numberOfThreads) {

    typedef tools::BlockStorage<LabelType> LabelStorage;
    // std::cout << "Start" << std::endl;
    // std::cout << "ignoreLabel " << ignoreLabel_ << std::endl;
    // std::cout << "Nthreads: " << numberOfThreads << std::endl;

    // set the number of nodes in the graph == number of labels
    BaseType::assign(numberOfLabels);

    // get the shape, number of slices and slice shapes
    const size_t nSlices = shape_[0];
    Coord2 sliceShape2({shape_[1], shape_[2]});
    Coord sliceShape3({1L, shape_[1], shape_[2]});

    // threadpool and actual number of threads
    nifty::parallel::ThreadPool threadpool(numberOfThreads);
    const std::size_t nThreads = threadpool.nThreads();

    std::vector<LabelType> minNodeInSlice(nSlices, numberOfLabels + 1);
    std::vector<LabelType> maxNodeInSlice(nSlices);

    // std::cout << "Before loop" << std::endl;
    // loop over the slices in parallel, for each slice find the edges
    // to nodes in the next 2 to 'range' slices
    {
        // instantiate the label storages
        LabelStorage labelsAStorage(threadpool, sliceShape3, nThreads);
        LabelStorage labelsBStorage(threadpool, sliceShape3, nThreads);

        parallel::parallel_foreach(threadpool, nSlices-2, [&](const int tid, const int slice) {
            // std::cout << "Loop in " << slice << std::endl;

            // get segmentation in base slice
            Coord beginA ({int64_t(slice), 0L, 0L});
            Coord endA({int64_t(slice + 1), shape_[1], shape_[2]});
            auto labelsA = labelsAStorage.getView(tid);
            tools::readSubarray(labels, beginA, endA, labelsA);
            auto labelsASqueezed = xtensor::squeezedView(labelsA);

            // iterate over the xy-coordinates and find the min and max nodes
            LabelType lU;
            LabelType & minNode = minNodeInSlice[slice];
            LabelType & maxNode = maxNodeInSlice[slice];
            tools::forEachCoordinate(sliceShape2, [&](const Coord2 coord){
                lU = xtensor::read(labelsASqueezed, coord.asStdArray());

                // if we have an ignore label, it is assumed to be zero and is
                // skipped in all the calculations
                if(lU == 0 && ignoreLabel_) {
                    return;
                }

                minNode = std::min(minNode, lU);
                maxNode = std::max(maxNode, lU);
            });
            // std::cout << minNode << " , " << maxNode << std::endl;

            // get view for segmenation in upper slice
            auto labelsB = labelsBStorage.getView(tid);

            // iterate over the next 2 - range_ slices
            for(int64_t z = 2; z <= range_; ++z) {

                // we continue if the long range affinity would reach out of the data
                if(slice + z >= shape_[0]) {
                    continue;
                }
                // std::cout << "to upper slice " << slice + z << std::endl;

                // get upper segmentation
                Coord beginB ({slice + z, 0L, 0L});
                Coord endB({slice + z + 1, shape_[1], shape_[2]});
                tools::readSubarray(labels, beginB, endB, labelsB);
                auto labelsBSqueezed = xtensor::squeezedView(labelsB);

                // iterate over the xy-coordinates and insert the long range edges
                LabelType lU, lV;
                tools::forEachCoordinate(sliceShape2, [&](const Coord2 coord){
                    lU = xtensor::read(labelsASqueezed, coord.asStdArray());
                    lV = xtensor::read(labelsBSqueezed, coord.asStdArray());

                    // skip ignore label
                    if(ignoreLabel_ && (lU == 0 || lV == 0)) {
                        return;
                    }

                    if(insertEdgeOnlyInNodeAdj(lU, lV)){
                        ++numberOfEdgesInSlice_[slice]; // if this is the first time we hit this edge, increase the edge count
                    }
                });
            }
        });
    }
    // std::cout << "Loop done" << std::endl;

    // set up the edge offsets
    size_t offset = numberOfEdgesInSlice_[0];
    {
        edgeOffset_[0] = 0;
        for(size_t slice = 1; slice < nSlices-2; ++slice) {
            edgeOffset_[slice] = offset;
            offset += numberOfEdgesInSlice_[slice];
        }
    }

    // set up the edge indices
    {
        auto & edges = BaseType::edges_;
        auto & nodes = BaseType::nodes_;
        edges.resize(offset);
        parallel::parallel_foreach(threadpool, nSlices-2,
                                   [&](const int tid, const int64_t slice){

            auto edgeIndex = edgeOffset_[slice];
            const auto startNode = minNodeInSlice[slice];
            const auto endNode   = maxNodeInSlice[slice] + 1;

            for(uint64_t u = startNode; u < endNode; ++u){
                for(auto & vAdj : nodes[u]){
                    const auto v = vAdj.node();
                    if(u < v){
                        auto e = BaseType::EdgeStorage(u, v);
                        edges[edgeIndex] = e;
                        vAdj.changeEdgeIndex(edgeIndex);
                        auto fres =  nodes[v].find(NodeAdjacency(u));
                        fres->changeEdgeIndex(edgeIndex);
                        // increase the edge index
                        ++edgeIndex;
                    }
                }
            }
        });
    }
    // std::cout << "Adjacency init done" << std::endl;
}

} // end namespace graph
} // end namespace nifty
