///   @file alloc2d.hpp
///   @brief functions that allocate and deallocate 2D arrays
///   @author Andrew Jewett
///   @date 2018-2-26

#ifndef _ALLOC2D_HPP
#define _ALLOC2D_HPP


namespace visfd {

/// @brief
/// Alloc2D() is a function for allocating 2-dimensional arrays of data
/// (contiguous in memory, in row-major format.)
/// (The functions in this file are used to allocate arrays for storing
///  tomographic data, and also precomputed filter-weights.)


template<typename Entry, typename Integer>

void Alloc2D(Integer const size[2], //!< size of the array in x,y directions
             Entry **paX,           //!< pointer to 1-D contiguous-memory array
             Entry ***paaX)         //!< pointer to 2-D multidimensional array
{
  if (! paX)
    return;

  // Allocate a 2-dimensional table row-major order
  // Optional: Also allocate a conventional 2-dimensional
  //           pointer-to-a-pointer-to-a-pointer data structure (aaX), that
  //           you can use to access the contents using aaX[j][i] notation.
  *paX = new Entry [size[0] * size[1]];

  if (! paaX)
    return;

  *paaX = new Entry* [size[1]];

  for(Integer iy=0; iy<size[1]; iy++)
    (*paaX)[iy] = &((*paX)[ iy*size[0] ]);

}


/// @brief
/// This function is the corresponding way to dellocate arrays
/// that were created using Alloc2D()

template<typename Entry, typename Integer>

void Dealloc2D(Integer const size[2], //!< size of the array in x,y directions
               Entry **paX,          //!< pointer to 1-D contiguous-memory array
               Entry ***paaX)        //!< pointer to 2-D multidimensional array
{
  if (paaX && *paaX) {
    delete [] (*paaX);
    *paaX = nullptr;
  }
  if (paX && *paX) {
    delete [] *paX;
    *paX = nullptr;
  }
}


} //namespace visfd


#endif //#ifndef _ALLOC2D_HPP
