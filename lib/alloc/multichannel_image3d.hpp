#ifndef _MULTICHANNEL_IMAGE3D_HPP
#define _MULTICHANNEL_IMAGE3D_HPP

template<class Scalar>

class CompactMultiChannelImage3D
{

private:

  Scalar **aafI;
  Scalar *afI;
  size_t n_good_voxels;
  int n_channels_per_voxel;
  int image_size[3];

public:

  Scalar ****aaaafI; // Stores the image data


  inline int
  nchannels() {
    return n_channels_per_voxel;
  }

  CompactMultiChannelImage3D(int set_n_channels_per_voxel) {
    n_channels_per_voxel = set_n_channels_per_voxel;
    n_good_voxels = 0;
    aaaafI = NULL;
  }

  CompactMultiChannelImage3D(int set_n_channels_per_voxel,
                             int const set_image_size[3],
                             Scalar const *const *const *aaafMask = NULL,
                             ostream *pReportProgress = NULL  //!< print progress to the user?
                             )
  {
    n_channels_per_voxel = set_n_channels_per_voxel;
    n_good_voxels = 0;
    aaaafI = NULL;
    Resize(image_size, aaafMask, pReportProgress);
  }

  inline void
  Resize(int const set_image_size[3],
         Scalar const *const *const *aaafMask = NULL,
         ostream *pReportProgress = NULL  //!< print progress to the user?
         )
  {
    if (aaaafI)
      Dealloc();
    Alloc(set_image_size, aaafMask, pReportProgress);
  }

  ~CompactMultiChannelImage3D() {
    Dealloc();
  }

private:

  inline void
  Alloc(int const set_image_size[3],
        Scalar const *const *const *aaafMask = NULL,
        ostream *pReportProgress = NULL  //!< print progress to the user?
        )
  {
    image_size[0] = set_image_size[0];
    image_size[1] = set_image_size[1];
    image_size[2] = set_image_size[2];

    if (pReportProgress)
      *pReportProgress
        << " -- Attempting to allocate space for a "
        << n_channels_per_voxel << "-channel image\n"
        << " -- (If this crashes your computer, find a computer with\n"
        << " --  more RAM and use \"ulimit\", OR use a smaller image.)\n";
    Alloc3D(image_size,
            &aafI,
            &aaaafI);
    for (int iz = 0; iz < image_size[2]; iz++)
      for (int iy = 0; iy < image_size[1]; iy++)
        for (int ix = 0; ix < image_size[0]; ix++)
          aaaafI[iz][iy][ix] = NULL;

    for (int iz = 0; iz < image_size[2]; iz++) {
      for (int iy = 0; iy < image_size[1]; iy++) {
        for (int ix = 0; ix < image_size[0]; ix++) {
          if (aaafMask && (aaafMask[iz][iy][ix] == 0.0))
            continue;
          n_good_voxels++;
        }
      }
    }
    afI = new Scalar[n_good_voxels * n_channels_per_voxel];
    int n = 0;
    for (int iz = 0; iz < image_size[2]; iz++) {
      for (int iy = 0; iy < image_size[1]; iy++) {
        for (int ix = 0; ix < image_size[0]; ix++) {
          if (aaafMask && (aaafMask[iz][iy][ix] == 0.0))
            continue;
          aaaafI[iz][iy][ix] =
            &(afI[n * n_channels_per_voxel]);
          n++;
        }
      }
    }

    if (pReportProgress)
      *pReportProgress
        << "        done\n" << endl;
  } //Alloc()


  inline void
  Dealloc()
  {
    delete [] afI;

    // Now delete the array of pointers (aaaafI, which pointed to afI)
    Dealloc3D(image_size,
              &aafI,
              &aaaafI);
    afI = NULL;
    aafI = NULL;
    aaaafI = NULL;
  }

}; //class CompactMultiChannelImage3D


#endif //#ifndef _MULTICHANNEL_IMAGE3D_HPP