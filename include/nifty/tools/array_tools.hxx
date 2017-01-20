#pragma once

#include <algorithm>
#include <vector>

#include "nifty/marray/marray.hxx"
#include "nifty/tools/for_each_coordinate.hxx"

namespace nifty {
namespace tools {


    // TODO one top function (unique) to select the appropriate unique version

    // unique values in array
    template<class T>
    inline void uniques(const marray::View<T> & array, std::vector<T> & out){
        
        out.resize(array.size());
        std::copy(array.begin(), array.end(), out.begin());
        
        std::sort(out.begin(),out.end());
        auto last = std::unique(out.begin(), out.end());
        out.erase( last, out.end() );
    }
    
    // unique values in masked array
    template<unsigned DIM, class T>
    inline void uniquesWithMask(const marray::View<T> & array, const marray::View<bool> & mask, std::vector<T> & out){
        //TODO check that array and mask have the same size
        
        typedef array::StaticArray<int64_t,DIM> Coord;
        Coord shape;
        for(int d = 0; d < DIM; ++d)
            shape[d] = array.shape(d);
            
        // copy if, but w.r.t. mask
        out.resize(array.size());
        auto itOut   = out.begin();
        
        forEachCoordinate(shape, [&](const Coord & coord){
            if(mask(coord.asStdArray())) {
                *itOut = array(coord.asStdArray()); 
                ++itOut;
            }
        });
        
        // resize the out vector
        out.resize(std::distance(out.begin(), itOut));
        
        std::sort(out.begin(),out.end());
        auto last = std::unique(out.begin(), out.end());
        out.erase( last, out.end() );
    }
    
    
    // unique values in array from coordinates
    template<unsigned DIM, class T>
    inline void uniquesWithCoordinates(const marray::View<T> & array,
            const std::vector<array::StaticArray<int64_t,DIM>> & coordinates,
            std::vector<T> & out){
        
        typedef array::StaticArray<int64_t,DIM> Coord;
        Coord shape;
        for(int d = 0; d < DIM; ++d)
            shape[d] = array.shape(d);
            
        out.resize(array.size());
        auto itOut   = out.begin();
        
        for(auto & coord : coordinates ) {
            *itOut = array(coord.asStdArray()); 
            ++itOut;
        }
        
        // resize the out vector
        out.resize(std::distance(out.begin(), itOut));
        
        std::sort(out.begin(),out.end());
        auto last = std::unique(out.begin(), out.end());
        out.erase( last, out.end() );
    }
    
    
    // unique values in masked array from coordinates
    template<unsigned DIM, class T>
    inline void uniquesWithMaskAndCoordinates(const marray::View<T> & array,
            const marray::View<bool> & mask, 
            const std::vector<array::StaticArray<int64_t,DIM>> & coordinates,
            std::vector<T> & out){
        //TODO check that array and mask have the same size
        
        typedef array::StaticArray<int64_t,DIM> Coord;
        Coord shape;
        for(int d = 0; d < DIM; ++d)
            shape[d] = array.shape(d);
            
        // copy if, but w.r.t. mask
        out.resize(array.size());
        auto itOut   = out.begin();
        
        for(auto & coord : coordinates ) {
            if(mask(coord.asStdArray())) {
                *itOut = array(coord.asStdArray()); 
                ++itOut;
            }
        }
        
        // resize the out vector
        out.resize(std::distance(out.begin(), itOut));
        
        std::sort(out.begin(),out.end());
        auto last = std::unique(out.begin(), out.end());
        out.erase( last, out.end() );
    }
    
    // coordinate where array == val
    template<unsigned DIM, class T>
    inline void 
    where(const marray::View<T> & array,
            const T val,
            std::vector<array::StaticArray<int64_t,DIM>> & coordsOut) {
        
        NIFTY_CHECK_OP(DIM,==,array.dimension(),"Dimensions do not match!");
        typedef array::StaticArray<int64_t,DIM> Coord;
        coordsOut.clear();

        Coord shape;
        for(int d = 0; d < DIM; ++d)
            shape[d] = array.shape(d);
        
        // reserve the max size for the vector
        coordsOut.reserve(array.size());

        forEachCoordinate(shape, [&](const Coord & coord){
            if(array(coord.asStdArray()) == val)
                coordsOut.emplace_back(coord);
        });
    }

    // coordinate where array == val
    // + returns bounding box
    template<unsigned DIM, class T>
    inline std::pair<array::StaticArray<int64_t,DIM>,array::StaticArray<int64_t,DIM>> 
    whereAndBoundingBox(const marray::View<T> & array,
            const T val,
            std::vector<array::StaticArray<int64_t,DIM>> & coordsOut) {
        
        NIFTY_CHECK_OP(DIM,==,array.dimension(),"Dimensions do not match!");
        typedef array::StaticArray<int64_t,DIM> Coord;
        coordsOut.clear();
        coordsOut.reserve(array.size());

        Coord shape;
        Coord bbBegin; // begin of bounding box
        Coord bbEnd;   // end of bounding box
        for(int d = 0; d < DIM; ++d) {
            shape[d] = array.shape(d);
            bbBegin[d] = array.shape(d);
            bbEnd[d] = 0;
        }

        forEachCoordinate(shape, [&](const Coord & coord){
            if(array(coord.asStdArray()) == val) {
                coordsOut.emplace_back(coord);
                for(int d = 0; d < DIM; ++ d) {
                    if(coord[d] < bbBegin[d])
                        bbBegin[d] = coord[d];
                    if(coord[d] > bbEnd[d]) 
                        bbEnd[d] = coord[d];
                }
            }
        });

        // increase end of the bounding box by 1 to have the actual bb coordinates
        for(int d = 0; d < DIM; ++d) {
            ++bbEnd[d];
        }

        return std::make_pair(bbBegin, bbEnd);
    }


} // namespace tools
} // namespace nifty
