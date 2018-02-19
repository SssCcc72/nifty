#ifdef WITH_HDF5

#include <pybind11/pybind11.h>
#include <iostream>

namespace py = pybind11;




namespace nifty{
namespace hdf5{


    void exportHdf5Common(py::module &);
    void exportHdf5Array(py::module &);
    //void exportHdf5BlockwiseWatershed(py::module &);
    void exportBenchmark(py::module &);
}
}




PYBIND11_MODULE(_hdf5, hdf5Module) {

    py::options options;
    options.disable_function_signatures();
    
    hdf5Module.doc() = "hdf5 submodule of nifty";

    using namespace nifty::hdf5;

    exportHdf5Common(hdf5Module);
    exportHdf5Array(hdf5Module);
    //exportHdf5BlockwiseWatershed(hdf5Module);
    exportBenchmark(hdf5Module);
}

#endif
