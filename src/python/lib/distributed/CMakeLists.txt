
addPythonModule(
    NESTED_NAME 
        nifty/distributed
    SOURCES
        distributed.cxx
        graph_extraction.cxx
        distributed_graph.cxx
        mergeable_features.cxx
        distributed_utils.cxx
        lifted_utils.cxx
        morphology.cxx
    LIBRRARIES
        # ${FASTFILTERS_LIBRARY}
        ${Z5_COMPRESSION_LIBRARIES}
        ${Boost_FILESYSTEM_LIBRARY}    
        ${Boost_SYSTEM_LIBRARY}    
        Threads::Threads
)
