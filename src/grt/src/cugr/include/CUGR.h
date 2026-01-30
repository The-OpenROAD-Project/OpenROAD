#pragma once

#include <csignal>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "grt/GRoute.h"
#include "odb/geom.h"

namespace odb {
class dbDatabase;
class dbNet;
class dbITerm;
class dbBTerm;
}  // namespace odb

namespace sta {
class dbSta;
class dbNetwork;
}  // namespace sta

namespace stt {
class SteinerTreeBuilder;
}  // namespace stt

namespace utl {
class Logger;
class CallBackHandler;
}  // namespace utl

namespace grt {

class Design;
class GridGraph;
class GRNet;
class BoxT;

struct Constants
{
  double weight_wire_length = 0.5;
  double weight_via_number = 4.0;
  double weight_short_area = 500.0;

  int min_routing_layer = 1;

  double cost_logistic_slope = 1.0;

  // allowed stem length increase to trunk length ratio
  double max_detour_ratio = 0.25;
  int target_detour_count = 20;

  double via_multiplier = 2.0;

  double maze_logistic_slope = 0.5;

  double pin_patch_threshold = 20.0;
  int pin_patch_padding = 1;
  double wire_patch_threshold = 2.0;
  double wire_patch_inflation_rate = 1.2;

  bool write_heatmap = false;
};

class CUGR
{
 public:
  CUGR(odb::dbDatabase* db,
       utl::Logger* log,
       utl::CallBackHandler* callback_handler,
       stt::SteinerTreeBuilder* stt_builder,
       sta::dbSta* sta);
  ~CUGR();
  void init(int min_routing_layer,
            int max_routing_layer,
            const std::set<odb::dbNet*>& clock_nets);
  void route();
  void write(const std::string& guide_file);
  NetRouteMap getRoutes();
  void updateDbCongestion();
  void getITermsAccessPoints(
      odb::dbNet* net,
      std::map<odb::dbITerm*, odb::Point3D>& access_points);
  void getBTermsAccessPoints(
      odb::dbNet* net,
      std::map<odb::dbBTerm*, odb::Point3D>& access_points);
  void setCriticalNetsPercentage(float percentage)
  {
    critical_nets_percentage_ = percentage;
  }

 private:
  float calculatePartialSlack();
  float getNetSlack(odb::dbNet* net);
  void setInitialNetSlacks();
  void updateOverflowNets(std::vector<int>& netIndices);
  void patternRoute(std::vector<int>& netIndices);
  void patternRouteWithDetours(std::vector<int>& netIndices);
  void mazeRoute(std::vector<int>& netIndices);
  void sortNetIndices(std::vector<int>& netIndices) const;
  void getGuides(const GRNet* net,
                 std::vector<std::pair<int, grt::BoxT>>& guides);
  void printStatistics() const;

  std::unique_ptr<Design> design_;
  std::unique_ptr<GridGraph> grid_graph_;
  std::vector<int> net_indices_;
  std::vector<std::unique_ptr<GRNet>> gr_nets_;
  std::map<odb::dbNet*, GRNet*> db_net_map_;

  odb::dbDatabase* db_;
  utl::Logger* logger_;
  utl::CallBackHandler* callback_handler_;
  stt::SteinerTreeBuilder* stt_builder_;
  sta::dbSta* sta_;
  NetRouteMap routes_;

  Constants constants_;

  int area_of_pin_patches_ = 0;
  int area_of_wire_patches_ = 0;

  float critical_nets_percentage_ = 0;
};

}  // namespace grt
