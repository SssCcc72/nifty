#include <cstddef>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "nifty/graph/rag/grid_rag_features.hxx"
#include "nifty/graph/rag/grid_rag_features_stacked.hxx"

#ifdef WITH_HDF5
#include "nifty/hdf5/hdf5_array.hxx"
#endif

#ifdef WITH_Z5
#include "nifty/z5/z5.hxx"
#endif

#include "xtensor-python/pytensor.hpp"
#include "xtensor-python/pyarray.hpp"


namespace py = pybind11;


namespace nifty{
namespace graph{

    using namespace py;


    // TODO parallelize
    template<class RAG, class T>
    void exportGridRagAccumulateLabelsT(py::module & ragModule){

        ragModule.def("gridRagAccumulateLabels",
            [](
                const RAG & rag,
                const xt::pyarray<T> & labels,
                const bool ignoreBackground,
                const T ignoreValue
            ){
                xt::pytensor<T, 1> nodeLabels = xt::zeros<T>({(int64_t) rag.numberOfNodes()});
                {
                    py::gil_scoped_release allowThreads;
                    gridRagAccumulateLabels(rag, labels, nodeLabels, ignoreBackground, ignoreValue);
                }
                return nodeLabels;

            },
            py::arg("graph"),
            py::arg("labels"),
            py::arg("ignoreBackground")=false,
            py::arg("ignoreValue")=0
        );
    }


    template<class RAG, class T, std::size_t DATA_DIM>
    void exportFindZExtendedNodesT(py::module & ragModule){

        ragModule.def("findZExtendedNodes",
            [](
                const RAG & rag
            ){
                std::vector<T> extendedNodes;
                {
                    py::gil_scoped_release allowThreads;
                    findZExtendedNodes(rag, extendedNodes);
                }
                return extendedNodes;

            },
            py::arg("graph")
        );
    }


    template<class RAG, class DATA>
    void exportGridRagStackedAccumulateLabelsT(py::module & ragModule){

        ragModule.def("gridRagAccumulateLabels",
            [](
                const RAG & rag,
                const DATA & labels,
                const int numberOfThreads
            ){
                typedef typename DATA::value_type DataType;
                xt::pytensor<DataType, 1> nodeLabels = xt::zeros<DataType>({(int64_t) rag.numberOfNodes()});
                {
                    py::gil_scoped_release allowThreads;
                    gridRagAccumulateLabels(rag, labels, nodeLabels);
                }
                return nodeLabels;

            },
            py::arg("graph"),
            py::arg("labels"),
            py::arg("numberOfThreads") = -1
        );
    }

    template<class RAG, class NODE_TYPE>
    void exportGetSkipEdgesForSliceT(
        py::module & ragModule
    ){
        ragModule.def("getSkipEdgesForSlice",
        [](
            const RAG & rag,
            const uint64_t z,
            std::map<std::size_t,std::vector<NODE_TYPE>> & defectNodes, // all defect nodes
            const bool lowerIsCompletelyDefected
        ){
            std::vector<std::size_t> deleteEdges;
            std::vector<std::size_t> ignoreEdges;

            std::vector<std::pair<NODE_TYPE,NODE_TYPE>> skipEdges;
            std::vector<std::size_t> skipRanges;
            {
                py::gil_scoped_release allowThreads;
                getSkipEdgesForSlice(
                    rag,
                    z,
                    defectNodes,
                    deleteEdges,
                    ignoreEdges,
                    skipEdges,
                    skipRanges,
                    lowerIsCompletelyDefected
                );
            }
            return std::make_tuple(deleteEdges, ignoreEdges, skipEdges, skipRanges);
        },
        py::arg("rag"),
        py::arg("z"),
        py::arg("defectNodes"),
        py::arg("lowerIsCompletelyDefected")
        );
    }

    void exportGraphAccumulator(py::module & ragModule) {

        // rag
        {
            typedef xt::pytensor<uint32_t, 2> ExplicitPyLabels2D;
            typedef GridRag<2, ExplicitPyLabels2D> ExplicitLabelsGridRag2D;

            typedef xt::pytensor<uint32_t, 3> ExplicitPyLabels3D;
            typedef GridRag<3, ExplicitPyLabels3D> ExplicitLabelsGridRag3D;

            // accumulate labels for uint8, uint32 and uint64
            exportGridRagAccumulateLabelsT<ExplicitLabelsGridRag2D, uint8_t>(ragModule);
            exportGridRagAccumulateLabelsT<ExplicitLabelsGridRag3D, uint8_t>(ragModule);

            exportGridRagAccumulateLabelsT<ExplicitLabelsGridRag2D, uint32_t>(ragModule);
            exportGridRagAccumulateLabelsT<ExplicitLabelsGridRag3D, uint32_t>(ragModule);

            exportGridRagAccumulateLabelsT<ExplicitLabelsGridRag2D, uint64_t>(ragModule);
            exportGridRagAccumulateLabelsT<ExplicitLabelsGridRag3D, uint64_t>(ragModule);
        }

        // explicit stacked rag
        {
            typedef xt::pytensor<uint32_t, 3> ExplicitPyLabels3D;
            typedef GridRagStacked2D<ExplicitPyLabels3D> StackedRagUInt32;

            typedef xt::pytensor<uint32_t, 3> UInt32Array;
            typedef xt::pytensor<uint64_t, 3> UInt64Array;

            // accumulate labels
            exportGridRagStackedAccumulateLabelsT<StackedRagUInt32, UInt32Array>(ragModule);
            exportGridRagStackedAccumulateLabelsT<StackedRagUInt32, UInt64Array>(ragModule);
        }

        // hdf5 stacked rag
        #ifdef WITH_HDF5
        {
            typedef nifty::hdf5::Hdf5Array<uint32_t> LabelsUInt32;
            typedef nifty::hdf5::Hdf5Array<uint64_t> LabelsUInt64;
            typedef GridRagStacked2D<LabelsUInt32> StackedRagUInt32;
            typedef GridRagStacked2D<LabelsUInt64> StackedRagUInt64;

            typedef nifty::hdf5::Hdf5Array<uint32_t> UInt32Array;
            typedef nifty::hdf5::Hdf5Array<uint64_t> UInt64Array;

            // accumulate labels
            exportGridRagStackedAccumulateLabelsT<StackedRagUInt32, UInt32Array>(ragModule);
            exportGridRagStackedAccumulateLabelsT<StackedRagUInt64, UInt32Array>(ragModule);
            exportGridRagStackedAccumulateLabelsT<StackedRagUInt32, UInt64Array>(ragModule);
            exportGridRagStackedAccumulateLabelsT<StackedRagUInt64, UInt64Array>(ragModule);

            // getSkipEdgesForSlice (deprecated)
            //exportGetSkipEdgesForSliceT<StackedRagUInt32,uint32_t>(ragModule);
            //exportGetSkipEdgesForSliceT<StackedRagUInt64,uint64_t>(ragModule);
        }
        #endif

        //n5 stacked rag
        #ifdef WITH_Z5
        {
            typedef nifty::nz5::DatasetWrapper<uint32_t> Labels32;
            typedef nifty::nz5::DatasetWrapper<uint64_t> Labels64;
            typedef GridRagStacked2D<Labels32> StackedRagUInt32;
            typedef GridRagStacked2D<Labels64> StackedRagUInt64;

            typedef nifty::nz5::DatasetWrapper<uint32_t> UInt32Array;
            typedef nifty::nz5::DatasetWrapper<uint64_t> UInt64Array;

            // accumulate labels
            exportGridRagStackedAccumulateLabelsT<StackedRagUInt32, UInt32Array>(ragModule);
            exportGridRagStackedAccumulateLabelsT<StackedRagUInt64, UInt32Array>(ragModule);

            exportGridRagStackedAccumulateLabelsT<StackedRagUInt32, UInt64Array>(ragModule);
            exportGridRagStackedAccumulateLabelsT<StackedRagUInt64, UInt64Array>(ragModule);

            // getSkipEdgesForSlice (deprecated)
            //exportGetSkipEdgesForSliceT<StackedRagUInt32,uint32_t>(ragModule);
            //exportGetSkipEdgesForSliceT<StackedRagUInt64,uint64_t>(ragModule);
        }
        #endif
    }

} // end namespace graph
} // end namespace nifty
