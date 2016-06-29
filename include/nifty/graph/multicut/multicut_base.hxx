#pragma once
#ifndef NIFTY_GRAPH_MULTICUT_MULTICUT_BASE_HXX
#define NIFTY_GRAPH_MULTICUT_MULTICUT_BASE_HXX

#include <string>
#include <initializer_list>
#include <sstream>
#include <stdexcept>
    
#include "nifty/graph/multicut/multicut_visitor_base.hxx"

namespace nifty {
namespace graph {



    class WeightsChangedNotSupported
    : public std::runtime_error{
    public:
        WeightsChangedNotSupported(const std::string msg = std::string())
        : std::runtime_error(msg){

        }
    };



    template<class OBJECTIVE>
    class MulticutBase{
    
    public:
        typedef OBJECTIVE Objective;
        typedef MulticutVisitorBase<Objective> VisitorBase;
        typedef MulticutVisitorProxy<Objective> VisitorProxy;
        typedef typename Objective::Graph Graph;
        typedef typename Graph:: template EdgeMap<uint8_t>  EdgeLabels;
        typedef typename Graph:: template NodeMap<uint64_t> NodeLabels;

        virtual ~MulticutBase(){};
        virtual void optimize(NodeLabels & nodeLabels, VisitorBase * visitor) = 0;
        virtual const Objective & objective() const = 0;
        virtual const NodeLabels & currentBestNodeLabels() = 0;


        virtual std::string name() const = 0 ;

        /**
         * @brief Inform solver about a change of weights
         * @details Inform solver that all weights could have changed. 
         * If a particular solver does not overload this function, a 
         * 
         */
        virtual void weightsChanged(){
            std::stringstream ss;
            ss<<this->name()<<" does not support changing weights";
            throw WeightsChangedNotSupported(ss.str());
        }   

        

        // with default implementation
        virtual double currentBestEnergy() {
            const auto & nl = this->currentBestNodeLabels();
            const auto & obj = this->objective();
            return obj.evalNodeLabels(nl);
        }



    };

} // namespace graph
} // namespace nifty

#endif // #ifndef NIFTY_GRAPH_MULTICUT_MULTICUT_BASE_HXX
