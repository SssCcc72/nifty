#include <pybind11/pybind11.h>
#include <iostream>

namespace py = pybind11;

PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);

namespace nifty{
namespace graph{


    void exportMulticutObjective(py::module &);
    void exportMulticutFactory(py::module &);
    void exportMulticutVisitorBase(py::module &);
    void exportMulticutBase(py::module &);
    void exportMulticutIlp(py::module &);
    void exportMulticutGreedyAdditive(py::module &);
    void exportFusionMoveBased(py::module &);


    void initSubmoduleMulticut(py::module &graphModule) {

        auto multicutModule = graphModule.def_submodule("multicut","multicut submodule");
        exportMulticutObjective(multicutModule);
        exportMulticutVisitorBase(multicutModule);
        exportMulticutBase(multicutModule);
        exportMulticutFactory(multicutModule);
        exportMulticutIlp(multicutModule);
        exportMulticutGreedyAdditive(multicutModule);
        exportFusionMoveBased(multicutModule);
    }

}
}
