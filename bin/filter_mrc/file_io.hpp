#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
using namespace std;
#include <err_report.hpp>

#include "settings.hpp"


/// @brief   Read a multi-column text file and store the words on each line
///          in a vector of vectors.  This function can also be used to
///          read a file containing multiple columns of numbers, for example
///          a list of 3D coordinates.  ("Entry" can be numeric.)
template<class Entry>
static void
ReadMulticolumnFile(istream &f,  //<! the file to be read
                    vector<vector<Entry> > &vvDest //<! store the results here
                    )
{
  vvDest.clear();
  size_t i_line = 1;
  while (f) {
    string strLine;
    getline(f, strLine);
    if (strLine.size() == 0)
      continue;
    stringstream ssLine(strLine);
    vector<Entry> row;
    Entry x;
    while (ssLine) {
      try {
        ssLine >> x;
      } // try {
      catch ( ... ) {
        stringstream err_msg;
        err_msg << "Error: File read error (invalid entry?) on line: "
                << i_line << "\n";
        throw InputErr(err_msg.str().c_str());
      }
      row.push_back(x);
    }
    i_line++;
  }
} // ReadMulticolumnFile()



/// @brief   Read a multi-column text file and store the words on each line
///          in a vector of vectors.  This function can also be used to
///          read a file containing multiple columns of numbers, for example
///          a list of 3D coordinates.  ("Entry" can be numeric.)
template<class Entry>
static void
ReadMulticolumnFile(string file_name,  //<! the file to be read
                    vector<vector<Entry> > &vvDest //<! store the results here
                    )
{
  fstream f;
  f.open(file_name.c_str(), ios::in);
  if (! f)
    throw InputErr("Error: unable to open \""+
                    file_name +"\" for reading.\n");
  ReadMulticolumnFile(f, vvDest);
  f.close();
}



/// @brief  Convert a vector-of-vectors of strings
///         (which typically represent the words on each line of a file)
///         with a vector-of-vectors of numbers.
///         The texton each line might be expressed using using IMOD notation:
///         "Pixel [91.3, 118.7, 231] = 134"
///         or using ordinary whitespace-delimited notation:
///         90.3 117.7 230.
///         If it is expressed in IMOD notation, subtract 1, 
///         but do not divide by the voxel_width.  (IMOD coordinates are in
///         units of voxels, not physical distance, however indices start at 1).
///         Otherwise, simply divide the numbers by the voxel width.

template<class Scalar, class Coordinate>
static void
ConvertStringsToCoordinates(const vector<vector<string> > &vvWords_orig, //<! words on each line of a file
                            vector<vector<Scalar> > &vvCoords, //<! convert these words to a matrix of numbers
                            Coordinate *voxel_width //<! scale factors.  Divide coordinates by these numbers
                            )
{
  assert(voxel_width);

  vvCoords.clear();

  bool is_output_from_imod = false;
  vector<vector<string> > vvWords = vvWords_orig;
  vector<Scalar> linked_set;

  for (size_t i = 0; i < vvWords.size(); i++)
  {
    if ((vvWords[i].size() > 0) && (vvWords[i][0] == "Pixel")) {
      vvWords.erase(vvWords.begin());
      is_output_from_imod = true;
    }
    vector<Scalar> coords_xyz;

    for (int d=0; d < vvWords[i].size(); d++) {
      // Get rid if weird characters enclosing or separating the words
      // such as '[', ']', or ','
      if ((vvWords[i][d].size() >= 0) && (vvWords[i][d][0] == '[')) {
        // Delete the '[' character from the beginning of the string.
        // (IMOD surrounds the coordinates with '[' and ']' characters.)
        vvWords[i][d] = vvWords[i][d].substr(1, string::npos);
        is_output_from_imod = true;
      }
      int n_chars;
      n_chars = vvWords[i][d].size();
      if ((vvWords[i][d].size()>=0) && (vvWords[i][d][n_chars-1]==']')) {
        // Delete the ']' character from the end of the string.
        // (IMOD surrounds the coordinates with '[' and ']' characters.)
        vvWords[i][d] = vvWords[i][d].substr(0, n_chars-1);
        is_output_from_imod = true;
      }
      n_chars = vvWords[i][d].size();
      if ((vvWords[i][d].size()>=0) && (vvWords[i][d][n_chars-1]==','))
        vvWords[i][d] = vvWords[i][d].substr(0, n_chars-1);

      // Now convert the remaining string to a number
      try {
        double x = stof(vvWords[i][d]); //<-- coordinate (x, y, or z)
      }
      catch ( ... ) {
        stringstream err_msg;
        err_msg << "Error: File read error (invalid entry?) on line: "
                << i+1 << "\n";
        throw InputErr(err_msg.str().c_str());
      }

      // The way we interpret the number depends on whether or not
      // we suspect the number was printed by IMOD, which uses
      // a somewhat unconventional coordinate system
      Scalar x;
      if (is_output_from_imod)
        x = floor(x) - 1.0;
      else
        x = floor((x / voxel_width[d]) + 0.5);
      // Finally, store the coordinate in "coords_xyz"
      coords_xyz.push_back(x);

    } //for (int d=0; d < vvWords[i].size(); d++)

    assert(coords_xyz.size() == vvWords.size());
    vvCoords.push_back(coords_xyz);

  } //for (size_t i = 0; i < vvWords.size(); i++)

} // ConvertStringsToCoordinates()





template<class Scalar>
static void
WriteOrientedPointCloudPLY(string filename,
                           vector<array<Scalar,3> > coords,
                           vector<array<Scalar,3> > norms)
{
  assert(coords.size() == norms.size());
  
  size_t n = coords.size();
  fstream ply_file;
  ply_file.open(filename.c_str(), ios::out);
  ply_file <<
    "ply\n"
    "format ascii 1.0\n"
    "comment  created by visfd\n"
    "element vertex " << n << "\n"
    "property float x\n"
    "property float y\n"
    "property float z\n"
    "property float nx\n"
    "property float ny\n"
    "property float nz\n"
    //"property list uchar int vertex_index\n" <- needed for "faces" only
    "end_header\n";
           
  for (size_t i=0; i < n; i++)
    ply_file << coords[i][0] << " "<< coords[i][1] << " " << coords[i][2] <<" "
             << norms[i][0] << " " << norms[i][1] << " " << norms[i][2] <<"\n";
  ply_file.close();
}




template<class Scalar>
static void
WriteOrientedPointCloudOBJ(string filename,
                           vector<array<Scalar,3> > coords,
                           vector<array<Scalar,3> > norms)
{
  assert(coords.size() == norms.size());
  
  size_t n = coords.size();
  fstream obj_file;
  obj_file.open(filename.c_str(), ios::out);
  obj_file << "# WaveFront *.obj file created by visfd\n";
  obj_file << "\n"
           << "g obj1_\n"
           << "\n";
           
  for (size_t i=0; i < n; i++)
    obj_file << "v "
             << coords[i][0] <<" "<< coords[i][1] <<" "<< coords[i][2] <<"\n";

  obj_file << "\n";

  for (size_t i=0; i < n; i++)
    obj_file << "vn "
             << norms[i][0] <<" "<< norms[i][1] <<" "<< norms[i][2] <<"\n";

  obj_file.close();
}



template<class Scalar>
static void
WriteOrientedPointCloudBNPTS(string filename,
                             vector<array<Scalar,3> > coords,
                             vector<array<Scalar,3> > norms)
{
  assert(coords.size() == norms.size());
  throw InputErr("The WriteOrientedPointCloudBNPTS() function is not working correctly.\n"
                 "No code should be invoking this function until the bug(s) is fixed.\n"
                 "If you are seeing this message, the error is the fault of the programmer.\n"
                 "Please contact the developer.");
  size_t n = coords.size();
  fstream bnpts_file;
  bnpts_file.open(filename.c_str(), ios::out | ios::binary);
  for (size_t i=0; i < n; i++) {
    float xyz[3];           //(convert from "Scalar" to float)
    xyz[0] = coords[i][0];  //(convert from "Scalar" to float)
    xyz[1] = coords[i][1];
    xyz[2] = coords[i][2];
    float norm[3];          //(convert from "Scalar" to float)
    norm[0] = norms[i][0];  //(convert from "Scalar" to float)
    norm[1] = norms[i][1];
    norm[2] = norms[i][2];
    bnpts_file.write((char*)&(xyz[0]), sizeof(float));
    bnpts_file.write((char*)&(xyz[1]), sizeof(float));
    bnpts_file.write((char*)&(xyz[2]), sizeof(float));
    bnpts_file.write((char*)&(norm[0]), sizeof(float));
    bnpts_file.write((char*)&(norm[1]), sizeof(float));
    bnpts_file.write((char*)&(norm[2]), sizeof(float));
  }
  bnpts_file.close();
}




template<class Scalar, class VectorContainer>
static void
WriteOrientedPointCloud(string pointcloud_file_name,
                        const int image_size[3],
                        VectorContainer const *const *const *aaaafVector,
                        Scalar const *const *const *aaafMask = NULL,
                        const Scalar *voxel_width=NULL)
{
  assert(aaaafVector);
  vector<array<Scalar,3> > coords;
  vector<array<Scalar,3> > norms;

  // Did the caller specify the physical width of each voxel?
  Scalar _voxel_width[3] = {1.0, 1.0, 1.0};
  if (voxel_width) {
    _voxel_width[0] = voxel_width[0];
    _voxel_width[1] = voxel_width[1];
    _voxel_width[2] = voxel_width[2];
  }

  for (int iz=0; iz < image_size[2]; iz++) {
    for (int iy=0; iy < image_size[1]; iy++) {
      for (int ix=0; ix < image_size[0]; ix++) {
        if (aaafMask && (aaafMask[iz][iy][ix] == 0.0))
          continue;

        array<Scalar,3> xyz;
        xyz[0] = ix * _voxel_width[0];
        xyz[1] = iy * _voxel_width[1];
        xyz[2] = iz * _voxel_width[2];
        coords.push_back(xyz);

        array<Scalar,3> norm;
        norm[0] = aaaafVector[iz][iy][ix][0];
        norm[1] = aaaafVector[iz][iy][ix][1];
        norm[2] = aaaafVector[iz][iy][ix][2];
        norms.push_back(norm);
      }
    }
  }

  //WriteOrientedPointCloudBNPTS(pointcloud_file_name + ".bnpts",
  //                             coords,
  //                             norms);

  //WriteOrientedPointCloudOBJ(pointcloud_file_name,
  //                           coords,
  //                           norms);

  WriteOrientedPointCloudPLY(pointcloud_file_name,
                             coords,
                             norms);

} //WriteOrientedPointCloud()





/// @brief   Parse the list of voxel coordinates provided 
///          by the user to use in "link" constraints.

template<class Scalar, class Coordinate>
static void
ProcessLinkConstraints(string must_link_filename,
                       vector<vector<array<Coordinate, 3> > > &must_link_constraints,
                       Scalar voxel_width[3])
{
  vector<vector<string> > vvWords;  // split file into lines and words
  vector<vector<float> > vvCoords; // replace each word with a number
  vector<array<int, 3> > linked_group;

  ReadMulticolumnFile(must_link_filename, vvWords); //read the words
  ConvertStringsToCoordinates(vvWords, //convert words to numbers
                              vvCoords,
                              voxel_width);

  // Now generate a vector of linked groups
  for (size_t i = 0; i < vvCoords.size(); i++) {
    if (vvCoords[i].size() == 0) {
      if (linked_group.size() > 0)
        must_link_constraints.push_back(linked_group);
      linked_group.clear();
    }
    else if (vvCoords[i].size() == 3) {
      array<int, 3> voxel_location;
      for (int d = 0; d < 3; d++)
        voxel_location[d] = vvCoords[i][d];
      linked_group.push_back(voxel_location);
    }
    else {
      stringstream err_msg;
      err_msg << "Error: Each line of file \""
              << must_link_filename << "\"\n"
              <<"       should contain either 3 numbers or 0 numbers.\n";
      throw InputErr(err_msg.str());
    }
  } //for (size_t i = 0; i < vvCoords.size(); i++)

  // Deal with any left-over linked_groups that we haven't added to the list yet
  if (linked_group.size() > 0)
    must_link_constraints.push_back(linked_group);

  if (must_link_constraints.size() == 0) {
    stringstream err_msg;
    err_msg << "Error: Format error in file \""<<must_link_filename<<"\".\n"
            << "       File contains no voxel coordinates.\n";
    throw InputErr(err_msg.str());
  }

  for (size_t i = 0; i < must_link_constraints.size(); i++) {
    linked_group = must_link_constraints[i];
    if ((linked_group.size() < 2) || (linked_group[0] == linked_group[1])) {
      stringstream err_msg;
      err_msg
        << "Error: Format error in file \""<<must_link_filename<<"\".\n"
        << "       Each broup must contain at least 2 voxels.  (Voxels appear on different\n"
        << "       lines, so blank-line delimters must not separate SINGLE non-blank lines)\n"
        << "       Furthermore, the voxels in each set must be unique.\n";

      throw InputErr(err_msg.str());
    }
  }

} //ProcessLinkConstraints()
