from __future__ import absolute_import
from . import _rag as __rag
from ._rag import *
from .. import Configuration

import numpy


__all__ = []
for key in __rag.__dict__.keys():
    try:
        __rag.__dict__[key].__module__='nifty.graph.rag'
    except:
        pass

    __all__.append(key)


def gridRag(labels, numberOfThreads=-1, serialization = None):

    labels = labels if numpy.issubdtype(labels.dtype, numpy.unsignedinteger) else numpy.require(labels, dtype=numpy.uint64)

    if labels.ndim == 2:
        if serialization is None:
            return explicitLabelsGridRag2D(labels, numberOfThreads=int(numberOfThreads))
        else:
            return explicitLabelsGridRag2D(labels, serialization)

    elif labels.ndim == 3:
        if serialization is None:
            return explicitLabelsGridRag3D(labels, numberOfThreads=int(numberOfThreads))
        else:
            return explicitLabelsGridRag3D(labels, serialization)
    else:
        raise RuntimeError("wrong dimension, currently only 2D and 3D is implemented")







if Configuration.WITH_HDF5:

    def gridRagHdf5(labels, numberOfLabels, blockShape = None, numberOfThreads=-1):

        dim = labels.ndim
        if blockShape is None:
            bs = [100]*dim
        else:
            bs = blockShape

        if dim == 2:
            labelsProxy = gridRag2DHdf5LabelsProxy(labels, int(numberOfLabels))
            ragGraph = gridRag2DHdf5(labelsProxy,bs,int(numberOfThreads))
        elif dim == 3:
            labelsProxy = gridRag3DHdf5LabelsProxy(labels, int(numberOfLabels))
            ragGraph = gridRag3DHdf5(labelsProxy,bs,int(numberOfThreads))
        else:
            raise RuntimeError("gridRagHdf5 is only implemented for 2D and 3D not for %dD"%dim)

        return ragGraph

    def gridRagStacked2DHdf5(labels, numberOfLabels, numberOfThreads=-1):
        dim = labels.ndim
        if dim == 3:
            labelsProxy = gridRag3DHdf5LabelsProxy(labels, int(numberOfLabels))
            ragGraph = gridRagStacked2DHdf5Impl(labelsProxy,int(numberOfThreads))
        else:
            raise RuntimeError("gridRagStacked2DHdf5 is only implemented for 3D not for %dD"%dim)

        return ragGraph

