#include <cstddef>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "xtensor-python/pytensor.hpp"

#include "nifty/graph/rag/grid_rag.hxx"
#include "nifty/graph/rag/project_to_pixels.hxx"
#include "nifty/graph/rag/project_to_pixels_stacked.hxx"


namespace py = pybind11;


namespace nifty{
namespace graph{



    using namespace py;

    template<class LABELS_PROXY, class T, std::size_t DATA_DIM>
    void exportProjectScalarNodeDataToPixelsT(py::module & ragModule){

        ragModule.def("projectScalarNodeDataToPixels",
           [](
                const GridRag<DATA_DIM, LABELS_PROXY> & rag,
                const xt::pytensor<T, 1> nodeData,
                const int numberOfThreads
           ){
                typedef typename xt::pytensor<T, DATA_DIM>::shape_type ShapeType;
                ShapeType shape;
                std::copy(rag.shape().begin(), rag.shape().end(), shape.begin());
                xt::pytensor<T, DATA_DIM> pixelData(shape);
                {
                    py::gil_scoped_release allowThreads;
                    projectScalarNodeDataToPixels(rag, nodeData, pixelData, numberOfThreads);
                }
                return pixelData;
           },
           py::arg("graph"),py::arg("nodeData"),py::arg("numberOfThreads")=-1
        );
    }


    template<class LABELS_PROXY, class T>
    void exportProjectScalarNodeDataToPixelsStackedT(py::module & ragModule){

        ragModule.def("projectScalarNodeDataToPixels",
           [](
                const GridRagStacked2D<LABELS_PROXY> & rag,
                const xt::pytensor<T, 1> nodeData,
                const int numberOfThreads
           ){
                typedef typename xt::pytensor<T, 3>::shape_type ShapeType;
                ShapeType shape;
                std::copy(rag.shape().begin(), rag.shape().end(), shape.begin());
                xt::pytensor<T, 3> pixelData(shape);
                {
                    py::gil_scoped_release allowThreads;
                    projectScalarNodeDataToPixels(rag, nodeData, pixelData, numberOfThreads);
                }
                return pixelData;
           },
           py::arg("graph"),py::arg("nodeData"),py::arg("numberOfThreads")=-1
        );
    }


    template<class LABELS_PROXY, class T, class PIXEL_DATA>
    void exportProjectScalarNodeDataToPixelsStackedOutOfCoreT(py::module & ragModule){

        ragModule.def("projectScalarNodeDataToPixels",
           [](
                const GridRagStacked2D<LABELS_PROXY> & rag,
                const xt::pytensor<T, 1> nodeData,
                PIXEL_DATA & pixelData,
                const int numberOfThreads
           ){
                const auto & shape = rag.shape();

                for(int d = 0; d < 3; ++d)
                    NIFTY_CHECK_OP(shape[d], ==, pixelData.shape()[d],
                                   "OutShape and Rag shape do not match!")
                {
                    py::gil_scoped_release allowThreads;
                    projectScalarNodeDataToPixels(rag, nodeData, pixelData, numberOfThreads);
                }
           },
           py::arg("graph"),
           py::arg("nodeData"),
           py::arg("pixelData"),
           py::arg("numberOfThreads")=-1
        );
    }


    template<class LABELS_PROXY, class T, class PIXEL_DATA>
    void exportProjectScalarNodeDataInSubBlockT(py::module & ragModule){

        ragModule.def("projectScalarNodeDataInSubBlock",
           [](
                const GridRagStacked2D<LABELS_PROXY> & rag,
                const std::map<T, T> & nodeData,
                PIXEL_DATA & pixelData,
                const std::vector<int64_t> & blockBegin,
                const std::vector<int64_t> & blockEnd,
                const int numberOfThreads
           ){
                const auto & shape = rag.shape();
                {
                    py::gil_scoped_release allowThreads;
                    projectScalarNodeDataInSubBlock(rag, nodeData, pixelData,
                                                    blockBegin, blockEnd,
                                                    numberOfThreads);
                }
           },
           py::arg("rag"),
           py::arg("nodeData"),
           py::arg("pixelData"),
           py::arg("blockBegin"),
           py::arg("blockEnd"),
           py::arg("numberOfThreads")=-1
        );
    }


    void exportProjectToPixels(py::module & ragModule) {

        // exportScalarNodeDataToPixels
        {
            typedef LabelsProxy<2, xt::pytensor<uint32_t, 2>> ExplicitPyLabels2D;
            typedef LabelsProxy<3, xt::pytensor<uint32_t, 3>> ExplicitPyLabels3D;

            exportProjectScalarNodeDataToPixelsT<ExplicitPyLabels2D, uint32_t, 2>(ragModule);
            exportProjectScalarNodeDataToPixelsT<ExplicitPyLabels3D, uint32_t, 3>(ragModule);

            exportProjectScalarNodeDataToPixelsT<ExplicitPyLabels2D, uint64_t, 2>(ragModule);
            exportProjectScalarNodeDataToPixelsT<ExplicitPyLabels3D, uint64_t, 3>(ragModule);

            exportProjectScalarNodeDataToPixelsT<ExplicitPyLabels2D, float, 2>(ragModule);
            exportProjectScalarNodeDataToPixelsT<ExplicitPyLabels3D, float, 3>(ragModule);

            exportProjectScalarNodeDataToPixelsT<ExplicitPyLabels2D, double, 2>(ragModule);
            exportProjectScalarNodeDataToPixelsT<ExplicitPyLabels3D, double, 3>(ragModule);
        }

        // exportScalarNodeDataToPixelsStacked
        {
            // explicit
            {
                typedef LabelsProxy<3, xt::pytensor<uint32_t, 3>> LabelsUInt32;

                exportProjectScalarNodeDataToPixelsStackedT<LabelsUInt32, uint32_t>(ragModule);
                exportProjectScalarNodeDataToPixelsStackedT<LabelsUInt32, uint64_t>(ragModule);
                exportProjectScalarNodeDataToPixelsStackedT<LabelsUInt32, float>(ragModule);
                exportProjectScalarNodeDataToPixelsStackedT<LabelsUInt32, double>(ragModule);
            }

            // hdf5
            #ifdef WITH_HDF5
            {
                typedef LabelsProxy<3, nifty::hdf5::Hdf5Array<uint32_t>> LabelsUInt32;

                // exports for uint 32 rag
                typedef nifty::hdf5::Hdf5Array<uint32_t> UInt32Data;
                exportProjectScalarNodeDataToPixelsStackedOutOfCoreT<LabelsUInt32,
                                                                     uint32_t,
                                                                     UInt32Data>(ragModule);

                typedef nifty::hdf5::Hdf5Array<uint64_t> UInt64Data;
                exportProjectScalarNodeDataToPixelsStackedOutOfCoreT<LabelsUInt32,
                                                                     uint64_t,
                                                                     UInt64Data>(ragModule);

                typedef nifty::hdf5::Hdf5Array<float> FloatData;
                exportProjectScalarNodeDataToPixelsStackedOutOfCoreT<LabelsUInt32,
                                                                     float,
                                                                     FloatData>(ragModule);

                typedef nifty::hdf5::Hdf5Array<double> DoubleData;
                exportProjectScalarNodeDataToPixelsStackedOutOfCoreT<LabelsUInt32,
                                                                     double,
                                                                     DoubleData>(ragModule);

                exportProjectScalarNodeDataInSubBlockT<LabelsUInt32,
                                                       uint32_t,
                                                       UInt32Data>(ragModule);
                exportProjectScalarNodeDataInSubBlockT<LabelsUInt32,
                                                       uint64_t,
                                                       UInt64Data>(ragModule);
            }
            #endif

            // z5
            #ifdef WITH_Z5
            {
                typedef LabelsProxy<3, nifty::nz5::DatasetWrapper<uint32_t>> LabelsUInt32;
                typedef LabelsProxy<3, nifty::nz5::DatasetWrapper<uint64_t>> LabelsUInt64;

                // exports for uinr 32 rag
                typedef nifty::nz5::DatasetWrapper<uint32_t> UInt32Data;
                exportProjectScalarNodeDataToPixelsStackedOutOfCoreT<LabelsUInt32,
                                                                     uint32_t,
                                                                     UInt32Data>(ragModule);

                typedef nifty::nz5::DatasetWrapper<uint64_t> UInt64Data;
                exportProjectScalarNodeDataToPixelsStackedOutOfCoreT<LabelsUInt32,
                                                                     uint64_t,
                                                                     UInt64Data>(ragModule);

                typedef nifty::nz5::DatasetWrapper<float> FloatData;
                exportProjectScalarNodeDataToPixelsStackedOutOfCoreT<LabelsUInt32,
                                                                     float,
                                                                     FloatData>(ragModule);

                typedef nifty::nz5::DatasetWrapper<double> DoubleData;
                exportProjectScalarNodeDataToPixelsStackedOutOfCoreT<LabelsUInt32,
                                                                     double,
                                                                     DoubleData>(ragModule);

                // exports for uint 64 rag
                typedef nifty::nz5::DatasetWrapper<uint32_t> UInt32Data;
                exportProjectScalarNodeDataToPixelsStackedOutOfCoreT<LabelsUInt64,
                                                                     uint32_t,
                                                                     UInt32Data>(ragModule);

                typedef nifty::nz5::DatasetWrapper<uint64_t> UInt64Data;
                exportProjectScalarNodeDataToPixelsStackedOutOfCoreT<LabelsUInt64,
                                                                     uint64_t,
                                                                     UInt64Data>(ragModule);

                typedef nifty::nz5::DatasetWrapper<float> FloatData;
                exportProjectScalarNodeDataToPixelsStackedOutOfCoreT<LabelsUInt64,
                                                                     float,
                                                                     FloatData>(ragModule);

                typedef nifty::nz5::DatasetWrapper<double> DoubleData;
                exportProjectScalarNodeDataToPixelsStackedOutOfCoreT<LabelsUInt64,
                                                                     double,
                                                                     DoubleData>(ragModule);

                //exportProjectScalarNodeDataInSubBlockT<LabelsUInt32,
                //                                       uint32_t,
                //                                       UInt32Data>(ragModule);
                //exportProjectScalarNodeDataInSubBlockT<LabelsUInt32,
                //                                       uint64_t,
                //                                       UInt64Data>(ragModule);
            }
            #endif
        }
    }

} // end namespace graph
} // end namespace nifty
