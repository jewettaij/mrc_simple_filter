#!/usr/bin/env bash

test_membrane_detection() {
    cd tests/
    OUT_FNAME_BASE="test_image_membrane_planar_55_tv_4_4_clust_1e+09_0.707_0.707_0.707_0.707"
    OUT_FNAME_REC=${OUT_FNAME_BASE}.rec
    OUT_FNAME_NORMALS=${OUT_FNAME_BASE}.ply
    ../bin/filter_mrc/filter_mrc -w 19.2 -w 19.2 -i test_image_membrane.rec -out ${OUT_FNAME_REC} -planar 55 -planar-tv 4 -planar-tv-angle-exponent 4 -connect-vector-saliency 0.707 -connect-vector-neighbor 0.707 -connect-tensor-saliency 0.707 -connect-tensor-neighbor 0.707 -connect-saliency 1e+09 -planar-normals-file ${OUT_FNAME_NORMALS} -select-cluster 1
    assertTrue "Failure during membrane detection, tensor-voting, or voxel clustering:  File \"${OUT_FNAME_REC}\" not created" "[ -s ${OUT_FNAME_REC} ]"
    SUM_VOXELS=`../bin/sum_voxels/sum_voxels "${OUT_FNAME_REC}"`
    assertTrue "Failure during membrane detection, tensor-voting, or voxel clustering:  File \"${OUT_FNAME_REC}\" has an incorrect checksum." "[ $SUM_VOXELS -eq 7687 ]"
    rm -rf "${OUT_FNAME_REC}" "${OUT_FNAME_NORMALS}"
  cd ../
}

. shunit2/shunit2
