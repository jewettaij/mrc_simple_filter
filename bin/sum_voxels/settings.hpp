#ifndef _SETTINGS_HPP
#define _SETTINGS_HPP

#define DISABLE_BOOTSTRAPPING
//#define DISABLE_TEMPLATE_MATCHING


#include<vector>
#include<string>
using namespace std;

class Settings {
 public:
  string in_file_name;
  bool rescale01_in;
  string mask_file_name;
  int mask_select;
  bool use_mask_select;

  bool calc_ave;
  bool calc_stddev;

  bool multiply_by_voxel_volume;
  float voxel_width;  //physical width of each voxel (for example in Angstroms)
  bool voxel_width_divide_by_10; //Use nm instead of Angstroms?

  bool use_thresholds;
  bool use_dual_thresholds;
  float in_threshold_01_a;
  float in_threshold_01_b;
  float in_threshold_10_a;
  float in_threshold_10_b;
  bool  in_thresh2_use_clipping;
  float in_thresh_a_value;
  float in_thresh_b_value;

  Settings();
  void ParseArgs(int argc, char **argv);

 private:
  void ParseArgs(vector<string>& vArgs);
  void ConvertArgvToVectorOfStrings(int argc, 
                                    char **argv, 
                                    vector<string>& dest);

}; //class Settings


#endif //#ifndef _SETTINGS_HPP
