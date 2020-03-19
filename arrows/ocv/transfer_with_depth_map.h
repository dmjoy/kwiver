/*ckwg +29
 * Copyright 2020 by Kitware, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of Kitware, Inc. nor the names of any contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef KWIVER_ARROWS_TRANSFER_WITH_DEPTH_MAP_H_
#define KWIVER_ARROWS_TRANSFER_WITH_DEPTH_MAP_H_

#include <arrows/ocv/kwiver_algo_ocv_export.h>
#include <arrows/ocv/image_container.h>

#include <vital/algo/detected_object_filter.h>
#include <vital/io/camera_io.h>

using namespace kwiver::vital;

namespace kwiver {
namespace arrows {
namespace ocv {

/// Transforms detections based on source and destination cameras.
class KWIVER_ALGO_OCV_EXPORT transfer_with_depth_map
  : public vital::algorithm_impl<transfer_with_depth_map,
                                 vital::algo::detected_object_filter>
{
public:
  PLUGIN_INFO( "transfer_with_depth_map",
               "Transforms a detected object set based on source and "
               "destination cameras with respect the source cameras depth "
               "map.\n\n" )

  /// Default constructor
  transfer_with_depth_map();

  /// Constructor taking source and destination cameras directly
  transfer_with_depth_map(kwiver::vital::camera_perspective_sptr src_cam,
                          kwiver::vital::camera_perspective_sptr dest_cam,
                          std::shared_ptr<kwiver::arrows::ocv::image_container> src_cam_depth_map);

  /// Get this algorithm's configuration block
  virtual vital::config_block_sptr get_configuration() const;

  /// Set this algorithm's properties via a config block
  virtual void set_configuration(vital::config_block_sptr config);

  /// Check that the algorithm's currently configuration is valid
  virtual bool check_configuration(vital::config_block_sptr config) const;

  /// Backproject image point to a depth map
  virtual vector_3d
    backproject_to_depth_map(kwiver::vital::camera_perspective_sptr const camera,
                             std::shared_ptr<kwiver::arrows::ocv::image_container> const depth_map,
                             vector_2d const& img_pt) const;

  /// Backproject an image point (top) assumed to be directly above another (bottom)
  virtual std::tuple<vector_3d, vector_3d>
    backproject_wrt_height(kwiver::vital::camera_perspective_sptr const camera,
                           std::shared_ptr<kwiver::arrows::ocv::image_container> const depth_map,
                           vector_2d const& img_pt_bottom,
                           vector_2d const& img_pt_top) const;

  /// Transfer a bounding box wrt source and destination cameras and a depth map
  virtual vital::bounding_box<double>
    transfer_bbox_with_depth_map(kwiver::vital::camera_perspective_sptr const src_camera,
                                 kwiver::vital::camera_perspective_sptr const dest_camera,
                                 std::shared_ptr<kwiver::arrows::ocv::image_container> const depth_map,
                                 vital::bounding_box<double> const bbox) const;

  /// Apply the transformation
  virtual vital::detected_object_set_sptr
    filter( vital::detected_object_set_sptr const input_set) const;

private:
  std::string src_camera_krtd_file_name;
  std::string dest_camera_krtd_file_name;
  std::string src_camera_depth_map_file_name;

  kwiver::vital::camera_perspective_sptr src_camera;
  kwiver::vital::camera_perspective_sptr dest_camera;
  std::shared_ptr<kwiver::arrows::ocv::image_container> depth_map;

  virtual int nearest_index(int max, double value) const;
};

}}} //End namespace

#endif // KWIVER_ARROWS_TRANSFER_WITH_DEPTH_MAP_H_