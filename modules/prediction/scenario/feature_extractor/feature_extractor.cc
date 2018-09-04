/******************************************************************************
 * Copyright 2018 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#include "modules/prediction/scenario/feature_extractor/feature_extractor.h"

#include <string>

#include "modules/common/adapters/proto/adapter_config.pb.h"
#include "modules/common/math/vec2d.h"
#include "modules/map/hdmap/hdmap_util.h"

using apollo::common::adapter::AdapterConfig;
using apollo::common::math::Vec2d;
using apollo::planning::ADCTrajectory;
using apollo::hdmap::HDMapUtil;
using apollo::hdmap::LaneInfo;
using apollo::hdmap::JunctionInfo;
using JunctionInfoPtr = std::shared_ptr<const JunctionInfo>;
using LaneInfoPtr = std::shared_ptr<const LaneInfo>;

namespace apollo {
namespace prediction {

FeatureExtractor::FeatureExtractor() {
  ego_trajectory_containter_ = dynamic_cast<ADCTrajectoryContainer*>(
      ContainerManager::instance()->GetContainer(
          AdapterConfig::PLANNING_TRAJECTORY));

  pose_container_ = dynamic_cast<PoseContainer*>(
      ContainerManager::instance()->GetContainer(
          AdapterConfig::LOCALIZATION));
}

FeatureExtractor::~FeatureExtractor() {
}

void FeatureExtractor::ExtractFeatures() {
  ExtractEgoVehicleFeatures();

  auto ego_trajectory_point = pose_container_->GetPosition();
  Vec2d ego_position(ego_trajectory_point.x(), ego_trajectory_point.y());

  ExtractEgoLaneFeatures(ego_position);

  ExtractFrontJunctionFeatures();
  // TODO(all) other processes
}

const ScenarioFeature& FeatureExtractor::GetScenarioFeatures() const {
  return scenario_feature_;
}

void FeatureExtractor::ExtractEgoVehicleFeatures() {
  // TODO(all): change this to ego_speed and ego_heading
  scenario_feature_.set_speed(pose_container_->GetSpeed());
  scenario_feature_.set_heading(pose_container_->GetTheta());
  // TODO(all): add acceleration if needed
}

void FeatureExtractor::ExtractEgoLaneFeatures(const Vec2d& ego_position) {
  LaneInfoPtr ego_lane_info = GetEgoLane(ego_position);
  if (ego_lane_info == nullptr) {
    AERROR << "Ego vehicle is not on any lane.";
    return;
  }
  scenario_feature_.set_curr_lane_id(ego_lane_info->id().id());
  double curr_lane_s = 0.0;
  double curr_lane_l = 0.0;
  ego_lane_info->GetProjection(ego_position, &curr_lane_s, &curr_lane_l);
  scenario_feature_.set_curr_lane_s(curr_lane_s);
}

void FeatureExtractor::ExtractNeighborLaneFeatures(const Vec2d& ego_position) {
}

void FeatureExtractor::ExtractFrontJunctionFeatures() {
  JunctionInfoPtr junction = ego_trajectory_containter_->ADCJunction();
  if (junction != nullptr) {
    scenario_feature_.set_junction_id(junction->id().id());
    scenario_feature_.set_dist_to_junction(
        ego_trajectory_containter_->ADCDistanceToJunction());
  }
}

void FeatureExtractor::ExtractObstacleFeatures() {
}


LaneInfoPtr FeatureExtractor::GetEgoLane(const Vec2d& ego_position) const {
  const auto& trajectory =
      ego_trajectory_containter_->adc_trajectory();
  for (const auto& lane_id : trajectory.lane_id()) {
    LaneInfoPtr lane_info =
        HDMapUtil::BaseMap().GetLaneById(hdmap::MakeMapId(lane_id.id()));
    if (lane_info->IsOnLane(ego_position)) {
      return lane_info;
    }
  }
  return nullptr;
}

std::vector<LaneInfoPtr> FeatureExtractor::GetNeighborLanes(
    const LaneInfoPtr& ego_lane_info) const {

}

}  // namespace prediction
}  // namespace apollo