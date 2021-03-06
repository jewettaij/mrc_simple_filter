///   @file draw.hpp
///   @brief a collection of functions for drawing objects
///          within an image for the purpose of annotation.
///   @author Andrew Jewett
///   @date 2019-4-15

#ifndef _DRAW_HPP
#define _DRAW_HPP

#include <cstring>
#include <cassert>
#include <cmath>
#include <limits>
#include <ostream>
#include <vector>
#include <tuple>
#include <set>
#include <queue>
using namespace std;
#include <err_visfd.hpp> // defines the "VisfdErr" exception type
#include <eigen3_simple.hpp>  // defines matrix diagonalizer (DiagonalizeSym3())
#include <visfd_utils.hpp>    // defines invert_permutation(), AveArray(), ...
#include <alloc2d.hpp>    // defines Alloc2D() and Dealloc2D()
#include <alloc3d.hpp>    // defines Alloc3D() and Dealloc3D()
#include <filter1d.hpp>   // defines "Filter1D" (used in ApplySeparable())
#include <filter3d.hpp>   // defines common 3D image filters



namespace visfd {



/// @brief  Create a 3D image containing multiple spheres (or spherical shells)
///         various sizes, thicknesses, and locations, specified by the caller.
///         The resulting spheres can (optionally) be superimposed with the
///         existing image (if the aaafBackground image array is != nullptr).

template<typename Scalar>

void
DrawSpheres(int const image_size[3], //!< image size
            Scalar ***aaafDest,  //!< array where we should write new image
            Scalar const *const *const *aaafMask,   //!< Optional: ignore voxels where mask==0
            const vector<array<Scalar,3> > &centers, //!< coordinates for the center of each sphere (blob)
            const vector<Scalar> *pDiameters=nullptr,         //!< Optional: diameter of each spherical shell (in voxels)
            const vector<Scalar> *pShellThicknesses=nullptr, //!< Optional: thickness of each spherical shell (in voxels)
            const vector<Scalar> *pVoxelIntensitiesForeground=nullptr, //!< Optional: assign voxels in spherical shell to this value (the vector should contain a separate entry for every sphere)
            Scalar voxel_intensity_background = 0.0, //!< Optional: assign background voxels to this value
            Scalar const *const *const *aaafBackground = nullptr,   //!< Optional: superimpose background image?
            Scalar voxel_intensity_background_rescale = 0.2, //!< Optional: superimpose with old image? This is the ratio of the fluctuations in voxel intensities of the newly created background image relative to the average foreground voxel intensity.
            bool voxel_intensity_foreground_normalize = false, //!< Optional: divide brightnesses by number of voxels in spherical shell? (rarely useful)
            ostream *pReportProgress = nullptr //!<Optional: report progress to the user?
            )
{
  assert(image_size);
  assert(aaafDest);
  // handle the edge cases first:

  if (centers.size() == 0)
    return;

  vector<Scalar> diameters;
  vector<Scalar> shell_thicknesses;
  vector<Scalar> voxel_intensities_foreground;
  
  if (pDiameters == nullptr) {
    // if empty, then fill the vector with the default value
    diameters.resize(centers.size(), 0.0);
    pDiameters = &diameters;
  }
  if (pShellThicknesses == nullptr) {
    // if empty, then fill the vector with the default value
    shell_thicknesses.resize(pDiameters->size());
    for (size_t i = 0; i < pDiameters->size(); i++)
      // By default, make each sphere a solid sphere by
      // setting the shell's thickness equal to the sphere's radius.
      shell_thicknesses[i] = (*pDiameters)[i] / 2;
    pShellThicknesses = &shell_thicknesses;
  }
  if (pVoxelIntensitiesForeground == nullptr) {
    // if empty, then fill the vector with the default foreground voxel value
    voxel_intensities_foreground.resize(centers.size(), 1.0);
    pVoxelIntensitiesForeground = &voxel_intensities_foreground;
  }
  assert(centers.size() == pDiameters->size());
  assert(centers.size() == pShellThicknesses->size());
  assert(centers.size() == pVoxelIntensitiesForeground->size());

  // rescale the brightnesses in the tomogram according to their average
  // value and standard deviation:

  Scalar background_ave = 0.0;
  Scalar background_stddev = 1.0;

  if (aaafBackground) {
    background_ave  =  AverageArr(image_size,
                                  aaafBackground,
                                  aaafMask);
    background_stddev  =  StdDevArr(image_size,
                                    aaafBackground,
                                    aaafMask);
  }
  double score_ave = 0.0;
  for (int i = 0; i < (*pVoxelIntensitiesForeground).size(); i++)
    score_ave += (*pVoxelIntensitiesForeground)[i];
  if ((*pVoxelIntensitiesForeground).size() > 0)
    score_ave /= (*pVoxelIntensitiesForeground).size();
  double score_rms = 0.0;
  for (int i = 0; i < (*pVoxelIntensitiesForeground).size(); i++)
    score_rms += SQR((*pVoxelIntensitiesForeground)[i]);
  if ((*pVoxelIntensitiesForeground).size() > 0)
    score_rms = sqrt(score_rms / (*pVoxelIntensitiesForeground).size());
  double voxel_intensity_foreground_rms = score_rms;

  // The spherical shells will have brightnesses chosen according to their
  // scores. Rescale the tomogram intensities so that they are approximately
  // in the range of scores.  This way we can see both at the same time
  // (when viewing the tomogram using IMOD, for example)
  for (int iz = 0; iz < image_size[2]; iz++) {
    for (int iy = 0; iy < image_size[1]; iy++) {
      for (int ix = 0; ix < image_size[0]; ix++) {
        if (background_stddev > 0.0)
          aaafDest[iz][iy][ix] =
            ((aaafBackground[iz][iy][ix] - background_ave) / background_stddev)
            *
            voxel_intensity_foreground_rms
            *
            voxel_intensity_background_rescale;
        else
          aaafDest[iz][iy][ix] = 0.0;
      }
    }
  }


  for (int iz=0; iz<image_size[2]; iz++)
    for (int iy=0; iy<image_size[1]; iy++)
      for (int ix=0; ix<image_size[0]; ix++)
        aaafDest[iz][iy][ix] += voxel_intensity_background;


  bool warn_points_outside_image = false;

  for (int i = 0; i < centers.size(); i++) {
    if (pReportProgress)
      *pReportProgress << "processing coordinates " << i+1 << " / "
           << centers.size() << ": x,y,z(in_voxels)="
           << centers[i][0] << "," << centers[i][1] << "," << centers[i][2];

    int ix = centers[i][0];
    int iy = centers[i][1];
    int iz = centers[i][2];

    if ((ix < 0) || (iy < 0) || (iz < 0) ||
        (ix >= image_size[0]) ||
        (iy >= image_size[1]) ||
        (iz >= image_size[2]))
      warn_points_outside_image = true;

    int Rs = ceil((*pDiameters)[i]/2-0.5);
    if (Rs < 0) Rs = 0;
    Scalar Rssqr_max = SQR((*pDiameters)[i]/2);
    Scalar Rssqr_min = 0.0;
    if (((*pShellThicknesses)[i] > 0.0) &&
        ((*pDiameters)[i]/2 - (*pShellThicknesses)[i] > 0.0))
      Rssqr_min = SQR((*pDiameters)[i]/2 - (*pShellThicknesses)[i]);

    if (pReportProgress)
      *pReportProgress << ", diameter=" << (*pDiameters)[i]
                       << ", th=" << (*pShellThicknesses)[i]
                       <<"\n";

    // Normalize the brightness of each sphere?
    // (ie by dividing the intensity by the number of voxels in the sphere)
    Scalar imultiplier = 1.0;
    long nvoxelspersphere = 1;
    if (voxel_intensity_foreground_normalize) {
      nvoxelspersphere = 0;
      for (int jz = -Rs; jz <= Rs; jz++) {
        for (int jy = -Rs; jy <= Rs; jy++) {
          for (int jx = -Rs; jx <= Rs; jx++) {
            if (aaafMask
                &&
                (aaafMask[static_cast<int>(iz)+jz]
                         [static_cast<int>(iy)+jy]
                         [static_cast<int>(ix)+jx]
                 == 0.0))
              continue;
            Scalar rsqr = jx*jx + jy*jy + jz*jz;
            if ((Rssqr_min <= rsqr) && (rsqr <= Rssqr_max))
              nvoxelspersphere++;
          }
        }
      }
    }
    imultiplier = 1.0;
    if (nvoxelspersphere > 0)
      imultiplier = 1.0 / nvoxelspersphere;
    for (int jz = -Rs; jz <= Rs; jz++) {
      for (int jy = -Rs; jy <= Rs; jy++) {
        for (int jx = -Rs; jx <= Rs; jx++) {
          int rsqr = jx*jx + jy*jy + jz*jz;
          if (! ((Rssqr_min <= rsqr) && (rsqr <= Rssqr_max)))
            continue;
          else if ((ix+jx < 0) ||
                   (ix+jx >= image_size[0]) ||
                   (iy+jy < 0) ||
                   (iy+jy >= image_size[1]) ||
                   (iz+jz < 0) ||
                   (iz+jz >= image_size[2]))
            continue;
          else if (aaafMask
                   &&
                   (aaafMask[iz+jz][iy+jy][ix+jx] == 0.0))
            continue;
          else
            aaafDest[iz+jz][iy+jy][ix+jx] =
              (*pVoxelIntensitiesForeground)[i] * imultiplier;
        }
      }
    }
  } //for (int i = 0; i < centers.size(); i++) {


  if (pReportProgress && warn_points_outside_image)
    *pReportProgress <<
      "------------------------------------------------------------------------------\n"
      "WARNING:\n"
      "    Some coordinates in the text file lie outside the boundaries of the image.\n"
      "    Did you remember to set the voxel width correctly?\n"
      "    (Did you use the \"-w\" argument?)\n"
      "--------------------------------------------------------------------------=---\n"
                     << endl;

} //DrawSpheres()



} //namespace visfd



#endif //#ifndef _DRAW_HPP
