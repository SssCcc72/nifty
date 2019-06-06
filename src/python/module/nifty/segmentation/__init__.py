import numpy as np
from ._segmentation import *
import time
from . import additional_algs


def get_valid_edges(shape, offsets, number_of_attractive_channels,
                    strides, randomize_strides, mask=None):
    # compute valid edges, i.e. the ones not going out of the image boundaries
    ndim = len(offsets[0])
    image_shape = shape[1:]
    valid_edges = np.ones(shape, dtype=bool)
    for i, offset in enumerate(offsets):
        for j, o in enumerate(offset):
            inv_slice = slice(0, -o) if o < 0 else slice(max(image_shape[j] - o,0), image_shape[j])
            invalid_slice = (i, ) + tuple(slice(None) if j != d else inv_slice
                                          for d in range(ndim))
            valid_edges[invalid_slice] = 0

    # mask additional edges if we have strides
    if strides is not None:
        assert len(strides) == ndim
        if randomize_strides:
            stride_factor = 1 / np.prod(strides)
            stride_edges = np.random.rand(*valid_edges.shape) < stride_factor
            stride_edges[:number_of_attractive_channels] = 1
            valid_edges = np.logical_and(valid_edges, stride_edges)
        else:
            stride_edges = np.zeros_like(valid_edges, dtype='bool')
            stride_edges[:number_of_attractive_channels] = 1
            valid_slice = (slice(number_of_attractive_channels, None),) +\
                tuple(slice(None, None, stride) for stride in strides)
            stride_edges[valid_slice] = 1
            valid_edges = np.logical_and(valid_edges, stride_edges)

    # if we have an external mask, mask all transitions to and within that mask
    if mask is not None:
        assert mask.shape == image_shape, "%s, %s" % (str(mask.shape), str(image_shape))
        assert mask.dtype == np.dtype('bool'), str(mask.dtype)
        # mask transitions to mask
        transition_to_mask, _ = compute_affinities(mask, offsets)
        transition_to_mask = transition_to_mask == 0
        valid_edges[transition_to_mask] = False
        # mask within mask
        valid_edges[:, mask] = False

    return valid_edges

def get_sorted_flat_indices_and_valid_edges(weights, offsets, number_of_attractive_channels,
                                            strides=None, randomize_strides=False, invert_repulsive_weights=True,
                                            bias_cut=0.):
    ndim = len(offsets[0])
    assert all(len(off) == ndim for off in offsets)
    image_shape = weights.shape[1:]

    valid_edges = get_valid_edges(weights.shape, offsets, number_of_attractive_channels,
                                  strides, randomize_strides)
    if invert_repulsive_weights:
        weights[number_of_attractive_channels:] *= -1
        weights[number_of_attractive_channels:] += 1
    weights[:number_of_attractive_channels] += bias_cut

    masked_weights = np.ma.masked_array(weights, mask=np.logical_not(valid_edges))

    tick = time.time()
    sorted_flat_indices = np.argsort(masked_weights, axis=None)[::-1]
    tock = time.time()
    print("Sorted edges in {}s".format(tock-tick))

    return valid_edges.ravel().astype('bool'), sorted_flat_indices.astype('uint64')

def run_mws(sorted_flat_indices,
                valid_edges,
                        offsets,
                        number_of_attractive_channels,
                        image_shape,
                        algorithm='kruskal'):
    assert algorithm in ('kruskal', 'divisive'), "Unsupported algorithm, %s" % algorithm
    if algorithm == 'kruskal':
        labels = compute_mws_segmentation_impl(sorted_flat_indices,
                                               valid_edges.ravel(),
                                               offsets,
                                               number_of_attractive_channels,
                                               image_shape)
    else:
        labels = compute_divisive_mws_segmentation_impl(sorted_flat_indices,
                                                    valid_edges.ravel(),
                                                    offsets,
                                                    number_of_attractive_channels,
                                                    image_shape)




def compute_mws_segmentation(weights, offsets, number_of_attractive_channels,
                             strides=None, randomize_strides=False, invert_repulsive_weights=True,
                             bias_cut=0., mask=None,
                             algorithm='kruskal'):
    assert algorithm in ('kruskal', 'prim'), "Unsupported algorithm, %s" % algorithm
    ndim = len(offsets[0])
    assert all(len(off) == ndim for off in offsets)
    image_shape = weights.shape[1:]

    # we assume that we get a 'valid mask', i.e. a mask where valid regions are set true
    # and invalid regions are set to false.
    # for computation, we need the opposite though
    inv_mask = None if mask is None else np.logical_not(mask)
    valid_edges = get_valid_edges(weights.shape, offsets, number_of_attractive_channels,
                                  strides, randomize_strides, inv_mask)

    # FIXME: double check if it is still necessary to invert weights (deleted from master)
    weights = np.copy(weights)
    if invert_repulsive_weights:
        weights[number_of_attractive_channels:] *= -1
        weights[number_of_attractive_channels:] += 1
    weights[:number_of_attractive_channels] += bias_cut

    if algorithm == 'kruskal' or algorithm == 'divisive':
        # sort and flatten weights
        # ignore masked weights during sorting
        masked_weights = np.ma.masked_array(weights, mask=np.logical_not(valid_edges))

        tick = time.time()
        sorted_flat_indices = np.argsort(masked_weights, axis=None)[::-1]
        tock = time.time()
        print("Sorted edges in {}s".format(tock-tick))

        # sorted_flat_indices = np.argsort(weights, axis=None)[::-1]
        if algorithm == 'kruskal':
            labels = compute_mws_segmentation_impl(sorted_flat_indices,
                                               valid_edges.ravel(),
                                               offsets,
                                               number_of_attractive_channels,
                                               image_shape)
        elif algorithm == 'divisive':
            labels = compute_divisive_mws_segmentation_impl(sorted_flat_indices,
                                                   valid_edges.ravel(),
                                                   offsets,
                                                   number_of_attractive_channels,
                                                   image_shape)
    else:
        labels = compute_mws_prim_segmentation_impl(weights.ravel(),
                                                    valid_edges.ravel(),
                                                    offsets,
                                                    number_of_attractive_channels,
                                                    image_shape)

    labels = labels.reshape(image_shape)
    # if we had an external mask, make sure it is mapped to zero
    if mask is not None:
        # increase labels by 1, so we don't merge anything with the mask
        labels += 1
        labels[inv_mask] = 0
    return labels
