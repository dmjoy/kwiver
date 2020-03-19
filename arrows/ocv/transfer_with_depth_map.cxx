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

#include "transfer_with_depth_map.h"

#include <math.h>
#include <assert.h>
#include <tuple>
#include <iostream>
#include <vital/io/camera_io.h>
#include <vital/config/config_difference.h>
#include <vital/util/string.h>
#include <Eigen/Core>
#include <arrows/ocv/image_container.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace kwiver::vital;

namespace kwiver {
namespace arrows {
namespace ocv {

// ---------------------------------------------------------------------------
transfer_with_depth_map::
transfer_with_depth_map()
{
}

// ---------------------------------------------------------------------------
transfer_with_depth_map::
transfer_with_depth_map
(kwiver::vital::camera_perspective_sptr src_cam,
 kwiver::vital::camera_perspective_sptr dest_cam,
 std::shared_ptr<kwiver::arrows::ocv::image_container> src_cam_depth_map)
  : src_camera( src_cam )
  , dest_camera( dest_cam )
  , depth_map( src_cam_depth_map )
{
}

// ---------------------------------------------------------------------------
vital::config_block_sptr
transfer_with_depth_map::
get_configuration() const
{
  // Get base config from base class
  vital::config_block_sptr config = vital::algorithm::get_configuration();

  config->set_value( "src_camera_krtd_file_name", src_camera_krtd_file_name,
                     "Source camera KRTD file name path" );

  config->set_value( "dest_camera_krtd_file_name", dest_camera_krtd_file_name,
                     "Destination camera KRTD file name path" );

  config->set_value( "src_camera_depth_map_file_name",
                     src_camera_depth_map_file_name,
                     "Source camera depth map file name path" );

  return config;
}

// ---------------------------------------------------------------------------
void
transfer_with_depth_map::
set_configuration( vital::config_block_sptr config_in )
{
  vital::config_block_sptr config = this->get_configuration();

  config->merge_config( config_in );
  this->src_camera_krtd_file_name =
    config->get_value< std::string > ( "src_camera_krtd_file_name" );
  this->dest_camera_krtd_file_name =
    config->get_value< std::string > ( "dest_camera_krtd_file_name" );
  this->src_camera_depth_map_file_name =
    config->get_value< std::string > ( "src_camera_depth_map_file_name" );


  this->src_camera =
    kwiver::vital::read_krtd_file( this->src_camera_krtd_file_name );
  this->dest_camera =
    kwiver::vital::read_krtd_file( this->dest_camera_krtd_file_name );

  cv::Mat src_cam_depth_map =
    cv::imread(src_camera_depth_map_file_name.c_str(), -1);
  this->depth_map = std::make_shared<kwiver::arrows::ocv::image_container>
    (src_cam_depth_map, kwiver::arrows::ocv::image_container::OTHER_COLOR);
}


// ---------------------------------------------------------------------------
bool
transfer_with_depth_map::
check_configuration( vital::config_block_sptr config ) const
{
  kwiver::vital::config_difference cd( this->get_configuration(), config );
  const auto key_list = cd.extra_keys();

  if ( ! key_list.empty() )
  {
    LOG_WARN( logger(), "Additional parameters found in config block that are "
                        "not required or desired: "
                        << kwiver::vital::join( key_list, ", " ) );
  }

  return true;
}

// ---------------------------------------------------------------------------
int
transfer_with_depth_map::
nearest_index(int max, double value) const
{
  if (abs(value - 0.0) < 1e-6)
  {
    return 0;
  }
  else if (abs(value - (double)max) < 1e-6)
  {
    return max - 1;
  }
  else
  {
    return (int)round(value - 0.5);
  }
}

// ---------------------------------------------------------------------------
vector_3d
transfer_with_depth_map::
backproject_to_depth_map
(kwiver::vital::camera_perspective_sptr const camera,
 std::shared_ptr<kwiver::arrows::ocv::image_container> const depth_map,
 vector_2d const& img_pt) const
{
  vector_2d npt_ = camera->intrinsics()->unmap(img_pt);
  auto npt = vector_3d(npt_(0), npt_(1), 1.0);

  matrix_3x3d M = camera->rotation().matrix().transpose();
  vector_3d cam_pos = camera->center();

  vector_3d Mp = M * npt;

  cv::Mat dm_data = depth_map->get_Mat();
  int dm_width = dm_data.cols;
  int dm_height = dm_data.rows;

  int img_pt_x = nearest_index(dm_width, img_pt(0));
  int img_pt_y = nearest_index(dm_height, img_pt(1));

  if (img_pt_x < 0 || img_pt_y < 0 ||
      img_pt_x >= dm_width ||
      img_pt_y >= dm_height)
  {
    LOG_ERROR( logger(), "Image point outside of image");
    abort();
  }

  float depth = dm_data.at<float>(img_pt_y, img_pt_x);

  vector_3d world_pos = cam_pos + (Mp * depth);

  return world_pos;
}

// ---------------------------------------------------------------------------
std::tuple<vector_3d, vector_3d>
transfer_with_depth_map::
backproject_wrt_height
(kwiver::vital::camera_perspective_sptr const camera,
 std::shared_ptr<kwiver::arrows::ocv::image_container> const depth_map,
 vector_2d const& img_pt_bottom,
 vector_2d const& img_pt_top) const
{
  vector_3d world_pos_bottom = backproject_to_depth_map
    (camera, depth_map, img_pt_bottom);

  vector_2d npt_ = camera->intrinsics()->unmap(img_pt_top);
  auto npt = vector_3d(npt_(0), npt_(1), 1.0);

  matrix_3x3d M = camera->rotation().matrix().transpose();
  vector_3d cam_pos = camera->center();

  vector_3d Mp = M * npt;

  double xf = world_pos_bottom(0);
  double yf = world_pos_bottom(1);

  double xc = cam_pos(0);
  double yc = cam_pos(1);
  double zc = cam_pos(2);

  double nx = Mp(0);
  double ny = Mp(1);
  double nz = Mp(2);

  double t = (ny * (yf - yc) + nx * (xf - xc)) /
    (std::pow(nx, 2) + std::pow(ny, 2));
  double zh = zc + t*nz;

  auto world_pos_top = vector_3d(xf, yf, zh);

  return std::tuple<vector_3d, vector_3d> (world_pos_bottom, world_pos_top);
}

// ---------------------------------------------------------------------------
vital::bounding_box<double>
transfer_with_depth_map::
transfer_bbox_with_depth_map
(kwiver::vital::camera_perspective_sptr const src_camera,
 kwiver::vital::camera_perspective_sptr const dest_camera,
 std::shared_ptr<kwiver::arrows::ocv::image_container> const depth_map,
 vital::bounding_box<double> const bbox) const
{
  double bbox_min_x = bbox.min_x();
  double bbox_max_x = bbox.max_x();
  double bbox_min_y = bbox.min_y();
  double bbox_max_y = bbox.max_y();
  double bbox_aspect_ratio = (bbox_max_x - bbox_min_x) /
    (bbox_max_y - bbox_min_y);

  auto bbox_bottom_center = vector_2d
    ((bbox_max_x + bbox_min_x) / 2, bbox_max_y);
  auto bbox_top_center = vector_2d
    ((bbox_max_x + bbox_min_x) / 2, bbox_min_y);

  vector_3d world_pos_bottom;
  vector_3d world_pos_top;

  std::tie(world_pos_bottom, world_pos_top) =
    backproject_wrt_height
    (src_camera, depth_map, bbox_bottom_center, bbox_top_center);

  vector_2d dest_img_pos_bottom = dest_camera->project(world_pos_bottom);
  vector_2d dest_img_pos_top = dest_camera->project(world_pos_top);

  double dest_bbox_min_y = dest_img_pos_top(1);
  double dest_bbox_max_y = dest_img_pos_bottom(1);
  double dest_bbox_height = dest_bbox_max_y - dest_bbox_min_y;

  double dest_bbox_width_d = bbox_aspect_ratio * dest_bbox_height;

  // Use the average center x coordinate of transfered top and bottom
  // points as the center of the bounding box
  double dest_bbox_min_x =
    ((dest_img_pos_top(0) + dest_img_pos_bottom(0)) / 2) -
    (dest_bbox_width_d / 2);
  double dest_bbox_max_x =
    ((dest_img_pos_top(0) + dest_img_pos_bottom(0)) / 2) +
    (dest_bbox_width_d / 2);

  return vital::bounding_box<double>
    (dest_bbox_min_x, dest_bbox_min_y, dest_bbox_max_x, dest_bbox_max_y);
}

// ---------------------------------------------------------------------------
vital::detected_object_set_sptr
transfer_with_depth_map::
filter( vital::detected_object_set_sptr const input_set ) const
{
  auto ret_set = std::make_shared<vital::detected_object_set>();

  for ( auto det : *input_set )
  {
    auto out_det = det->clone();
    auto out_bbox = out_det->bounding_box();
    vital::bounding_box<double> new_out_bbox = transfer_bbox_with_depth_map
      (src_camera, dest_camera, depth_map, out_bbox);
    out_det->set_bounding_box( new_out_bbox );
    ret_set->add( out_det );
  }

  return ret_set;
}

}}} // end namespace
