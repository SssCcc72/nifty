#include <pybind11/pybind11.h>
#include <iostream>

namespace py = pybind11;

PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);

namespace nifty{
namespace graph{
namespace opt{
namespace mincut{

    void exportMincutObjective(py::module &);
    void exportMincutFactory(py::module &);
    void exportMincutVisitorBase(py::module &);
    void exportMincutBase(py::module &);
    #if WITH_QPBO
    void exportMincutQpbo(py::module &);
    #endif 

    void exportMincutCcFusionMoveBased(py::module &);
    #if WITH_QPBO
    void exportMincutGreedyAdditive(py::module &);
    #endif 

} // namespace nifty::graph::opt::mincut
} // namespace nifty::graph::opt
}
}




PYBIND11_MODULE(_mincut, mincutModule) {

    py::options options;
    options.disable_function_signatures();
    
    mincutModule.doc() = "mincut submodule of nifty.graph";
    
    using namespace nifty::graph::opt::mincut;

    exportMincutObjective(mincutModule);
    exportMincutVisitorBase(mincutModule);
    exportMincutBase(mincutModule);
    exportMincutFactory(mincutModule);
    #ifdef WITH_QPBO
    exportMincutQpbo(mincutModule);
    exportMincutGreedyAdditive(mincutModule);
    #endif
    exportMincutCcFusionMoveBased(mincutModule);

}

