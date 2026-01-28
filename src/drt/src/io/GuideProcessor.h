// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <queue>
#include <set>
#include <utility>
#include <vector>

#include "boost/icl/interval_set.hpp"
#include "db/infra/frPoint.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frInstTerm.h"
#include "db/tech/frTechObject.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "global.h"
#include "odb/db.h"
#include "utl/Logger.h"

namespace drt::io {

using TrackIntervals = std::map<frCoord, boost::icl::interval_set<frCoord>>;
using TrackIntervalsByLayer = std::vector<TrackIntervals>;

class GuideProcessor
{
 public:
  GuideProcessor(frDesign* designIn,
                 odb::dbDatabase* dbIn,
                 utl::Logger* loggerIn,
                 RouterConfiguration* router_cfg)
      : design_(designIn), logger_(loggerIn), db_(dbIn), router_cfg_(router_cfg)
  {
  }
  /**
   * @brief Reads guides from odb and fill the tmp_guides_ list of unprocessed
   * guides
   */
  bool readGuides();
  /**
   * @brief Creates the GCELLGRID from either the DEF or the guides.
   */
  void buildGCellPatterns();
  /**
   * Processes the guides, checks their correctness, and clean them up.
   * @throws Error if the guides are not correct.
   */
  void processGuides();

 private:
  // getters
  frTechObject* getTech() const { return design_->getTech(); }
  frDesign* getDesign() const { return design_; }
  // processGuides helper functions
  void buildGCellPatterns_getWidth(frCoord& GCELLGRIDX, frCoord& GCELLGRIDY);
  void buildGCellPatterns_getOffset(frCoord GCELLGRIDX,
                                    frCoord GCELLGRIDY,
                                    frCoord& GCELLOFFSETX,
                                    frCoord& GCELLOFFSETY);
  void buildGCellPatterns_helper(frCoord& GCELLGRIDX,
                                 frCoord& GCELLGRIDY,
                                 frCoord& GCELLOFFSETX,
                                 frCoord& GCELLOFFSETY);

  std::vector<std::pair<frBlockObject*, odb::Point>> genGuides(
      frNet* net,
      std::vector<frRect> rects);
  void genGuides_addCoverGuide(frNet* net, std::vector<frRect>& rects);
  void genGuides_addCoverGuide_helper(frInstTerm* term,
                                      std::vector<frRect>& rects);
  /**
   * @brief Creates/Extends guides to cover a pin shape at best_pin_loc_coords
   * through its closest guide.
   *
   * This is a helper function to patchGuides() function. it extends/bloats the
   * guide at closest guide index in order to connect to best_pin_loc_coords. It
   * also creates new guides to connect the chosen closest guide with the
   * best_pin_loc_coords if needed.
   * @param net the current net whose guides we are processing
   * @param guides list of gr guides of the net
   * @param best_pin_loc_idx The index of the gcell holding the major part of
   * the best/chosen pin shape. Its z coordinate is the layerNum of the chosen
   * pin shape
   * @param closest_guide_idx The index of the guide that is closest to the
   * best_pin_loc_coords in the guides list
   */
  void patchGuides_helper(frNet* net,
                          std::vector<frRect>& guides,
                          const Point3D& best_pin_loc_idx,
                          const Point3D& best_pin_loc_coords,
                          int closest_guide_idx);
  /**
   * @brief Connects the guides with the best pin shape location (on the 2D
   * plane only)
   *
   * The function creates a patch guide that connects the closest guide to
   * best_pin_loc_coords (without consideration to different layers)
   *
   * @param guide_pt The center of the gcell on the guide that is closest to
   * best_pin_loc_coords
   * @param best_pin_loc_coords The gcell center point of the chosen pin shape
   * @param gcell_half_size_horz Half the horizontal size of the gcell
   * @param gcell_half_size_vert Half the vertical size of the gcell
   */
  void connectGuidesWithBestPinLoc(Point3D& guide_pt,
                                   const odb::Point& best_pin_loc_coords,
                                   frCoord gcell_half_size_horz,
                                   frCoord gcell_half_size_vert,
                                   frNet* net,
                                   std::vector<frRect>& guides);
  /**
   * @brief Patches guides to cover part of the pin if needed.
   *
   * The function checks all the pin shapes against the guides to see if any of
   * them overlap with the guides. If not, it extends the existing guides to
   * overlap with a pin shape.
   * @param net the current net whose guides we are processing
   * @param pin a pin in the net which we are attempting to connect to the
   * guides
   * @param guides list of gr guides of the net
   */
  void patchGuides(frNet* net, frBlockObject* pin, std::vector<frRect>& guides);
  /**
   * @brief Adds patch guides to cover net pins if needed.
   *
   * The function calls `patchGuides` for all net pins
   * @param net the current net whose pins we are checking with the nets guides
   * @param guides list of gr guides of the net before any processing. The list
   * is modified by patchGuides if needed.
   */
  void coverPins(frNet* net, std::vector<frRect>& guides);
  /**
   * @brief Prepares guides for a star traversal
   *
   * The function transforms guides to an easier to manage data strcture;
   * TrackIntervalsByLayer. For each layer, it adds track indicies, where there
   * are guides, to intvs map. Each entry in the map has a set of intervals of
   * the indices where the guides span on. By construction, the function merges
   * touching guides; for example it merges a guide on vertical index 1 that
   * spans from horizontal index 5 to 10 with a guide on the same vertical index
   * spanning from 10 to 15 into 1 entry on vertical index 1 with begin index 5
   * and end index 15. It also manages touching guides on consecutive tracks by
   * adding a bridge guide on the upper or lower layer that connects them if
   * needed.
   * @note The index is the gcell index.
   * @param rects list of guides of the current net.
   * @param intvs vector of trackIntervals. TrackIntervals key is the track
   * index (vertically or horizontally depending on the layer direction).
   * TrackIntervals value is a set of intervals of indices.
   */
  void genGuides_prep(const std::vector<frRect>& rects,
                      TrackIntervalsByLayer& intvs);
  /**
   * Initializes list of guides for graph traversal.
   *
   * The function follows the given intvs calculated by `genGuides_prep` and
   * intializes the list guides that is going to be used for graph traversal. It
   * splits the intvs by pin indices and layer intersection. For example, assume
   * we have interval on horizontal track y=5 on range [0, 10]. if there exists
   * a pin on gcell index (2,5), the intv should be split to [0,2] and [2,10].
   * Same if there exists an interval on the higher or layer layer at track x=2
   * and range[0,10]. If we are in the first iteration, we consider guides below
   * or on VIA_ACCESS_LAYER_NUM as via guides only. This function also populates
   * the pin_gcell_map.
   *
   * @param rects A list to store the resulting guides from this function.
   * @param intvs The track intervals calculated by genGuides_prep
   * @param gcell_pin_map Map from gcell index to pins.
   * @param pin_gcell_map Map from pin object to gcell indices.
   * @param via_access_only True if this the first iteration. Indicates that
   * splitting is gonna consider Guides on VIA_ACCESS_LAYER_NUM or below as via
   * guides only.
   *
   */
  void genGuides_split(std::vector<frRect>& rects,
                       const TrackIntervalsByLayer& intvs,
                       const std::map<Point3D, frBlockObjectSet>& gcell_pin_map,
                       frBlockObjectMap<std::set<Point3D>>& pin_gcell_map,
                       bool via_access_only) const;
  /**
   * Initializes a map of gcell location to set of pins
   *
   * The function maps each gcell index to a set of pins that are to be
   * considered part of the gcell.
   *
   * @param net The current net being processes. Its pins are the ones
   * considered by this function.
   * @param gcell_pin_map A map to be populated by the result of this function.
   */
  void initGCellPinMap(
      const frNet* net,
      std::map<Point3D, frBlockObjectSet>& gcell_pin_map) const;
  /**
   * Populates gcell_pin_map with the values associated with the passed term
   * based on pin shapes.
   *
   * This is a helper function to initGCellPinMap. It loops over all pin shapes.
   * It maps all the gcell indices that any of the pin shapes touches to the
   * current pin.
   *
   * @param gcell_pin_map The map to be populated with the results.
   * @param term The current pin we are processing.
   */
  void mapPinShapesToGCells(std::map<Point3D, frBlockObjectSet>& gcell_pin_map,
                            frBlockObject* term) const;
  /**
   * Populates gcell_pin_map with the values associated with the passed pin
   * based on access points.
   *
   * Does a similar job to `mapPinShapesToGCells`. But instead of relying on
   * pin shapes and their intersection with gcells, it relies on the preferred
   * access point of each pin shape.
   *
   * @param gcell_pin_map The map to be populated with the results.
   * @param term The current pin we are processing.
   */
  void mapTermAccessPointsToGCells(
      std::map<Point3D, frBlockObjectSet>& gcell_pin_map,
      frBlockObject* pin) const;

  void initPinGCellMap(frNet* net,
                       frBlockObjectMap<std::set<Point3D>>& pin_gcell_map);
  // write guide
  void saveGuidesUpdates();

  /**
   * @brief Reads the GCell grid information from the database
   * This function should be called before processing guides to ensure
   * we have all necessary grid information from the database.
   */
  void readGCellGrid();

  /**
   * @brief Checks the validity of the odb guide layer
   *
   * The db_guide layer is invalid if it is any of the following conditions:
   * - Not in the DRT layer database
   * - Above the sepecified top routing layer
   * - Below the specified bottom routing layer and the via access layer
   * @note If a layer is invalid, this produces an error unless it is above the
   * top routing layer for a net that has pins above the top routing layer. In
   * the latest case, we just ignore the guide and the pin is handled by
   * io::Parser::setBTerms_addPinFig_helper
   * @param db_guide The guide to check
   * @param net The net containing the guide
   * @param layer_num The layer_num of the guide returned by this function if it
   * is a valid layer
   * @returns True if the guide is valid by the previous criteria and False
   * if above top routing layer for a net with bterms above top routing layer
   */
  bool isValidGuideLayerNum(odb::dbGuide* db_guide,
                            frNet* net,
                            frLayerNum& layer_num);

  frDesign* design_;
  utl::Logger* logger_;
  odb::dbDatabase* db_;
  RouterConfiguration* router_cfg_;
  frOrderedIdMap<frNet*, std::vector<frRect>> tmp_guides_;
};

/**
 * @brief A class for traversing guides graph to check pin coverage and minimize
 * guides.
 *
 * The GuidePathFinder class helps in finding paths between guides and pins on a
 * net. It works by constructing adjacency lists of guides
 * and pins, and then traverses the graph to find the minimal set of guides that
 * visit all pins. It commits the found path to the final guides of the net.
 */
class GuidePathFinder
{
 public:
  /**
   * @brief Constructs the object and initializes member variables
   *
   * @param rects A vector of guide rectangles (by GCell indices).
   * @param pin_gcell_map A map of pins and their corresponding GCell indices.
   */
  GuidePathFinder(frDesign* design,
                  utl::Logger* logger,
                  RouterConfiguration* router_cfg,
                  frNet* net,
                  bool force_feed_through,
                  const std::vector<frRect>& rects,
                  const frBlockObjectMap<std::set<Point3D>>& pin_gcell_map);
  int getNodeCount() const { return node_count_; }
  int getGuideCount() const { return guide_count_; }
  int getPinCount() const { return node_count_ - guide_count_; }
  bool isForceFeedThrough() const { return force_feed_through_; }
  bool allowWarnings() const { return allow_warnings_; }
  void setForceFeedThrough(const bool value) { force_feed_through_ = value; }
  void setAllowWarnings(const bool value) { allow_warnings_ = value; }
  /**
   * @brief Traverses the graph to find paths between pins and guides.
   *
   * The function uses a modified BFS to traverse the constructed graph
   * defined by adj_list_ untill it visits all pins in the graph.
   *
   * @return True if all pins are visited, false otherwise.
   */
  bool traverseGraph();

  /**
   * @brief Connects disconnected guides through adding a patch bridge guide.
   */
  void connectDisconnectedComponents(const std::vector<frRect>& rects,
                                     TrackIntervalsByLayer& intvs);

  /**
   * @brief Writes the final resulting set of guides to the net and updates the
   * GRPins.
   *
   * The function considers only the minimal set of guides found by
   * `traverseGraph` for the final guide update. It cleans up those guides and
   * commits them to the net. It also populates gr_pins list with the pins
   * alongside the gcell centers that they were accessed from by
   * `traverseGraph`.
   * @note The guides added by this function are of 0 width as they are actually
   * a line connecting two gcell centers.
   *
   * @param rects A vector of guide rectangles.
   * @param pin_gcell_map A map of pins and their corresponding GCell indices.
   * @returns A vector of pin-gcell pair to be updated.
   */
  std::vector<std::pair<frBlockObject*, odb::Point>> commitPathToGuides(
      std::vector<frRect>& rects,
      const frBlockObjectMap<std::set<Point3D>>& pin_gcell_map);

 private:
  struct Wavefront
  {
    int node_idx;
    int prev_idx;
    int cost;
    bool operator<(const Wavefront& b) const
    {
      if (cost == b.cost) {
        return node_idx > b.node_idx;
      }
      return cost > b.cost;
    }
  };
  /**
   * @brief Builds the node map and other member variables.
   *
   * The function constructs the node map from the given list of guides and
   * pins.
   *
   * @param rects A vector of guide rectangles(by GCell indices).
   * @param pin_gcell_map A map of pins and their corresponding GCell indices.
   */
  void buildNodeMap(const std::vector<frRect>& rects,
                    const frBlockObjectMap<std::set<Point3D>>& pin_gcell_map);
  /**
   * @brief Constructs the adjacency list for the graph of guides and pins.
   *
   * This function builds the graph by creating connections (edges) between
   * guides and pins, ensuring that the correct links are made for routing.
   */
  void constructAdjList();
  /**
   * @brief Checks if the given node index corresponds to a pin.
   *
   * @param idx The index to check.
   * @return True if the index corresponds to a pin, false otherwise.
   */
  bool isPinIdx(const int idx) const { return idx >= guide_count_; }
  /**
   * @brief Checks if the given node index corresponds to a guide.
   *
   * @param idx The index to check.
   * @return True if the index corresponds to a guide, false otherwise.
   */
  bool isGuideIdx(const int idx) const
  {
    return idx > -1 && idx < guide_count_;
  }
  /**
   * @brief Converts a pin index to its true pin index.
   *
   * @param idx The index to convert.
   * @return The true pin index.
   */
  int getTruePinIdx(const int idx) const { return idx - guide_count_; }
  /**
   * @brief Initializes the search queue with nodes for traversal.
   *
   * This function creates the initial priority queue for the graph traversal,
   * either starting from the first pin or all previously visited nodes.
   *
   * @return A priority queue of Wavefront objects, representing nodes to
   * explore during traversal.
   */
  std::priority_queue<Wavefront> getInitSearchQueue();
  /**
   * @brief Creates a mapping of pins to GCell locations.
   *
   * This function processes the provided guide rectangles and pin-to-GCell
   * mapping to create a list of GCell locations for each pin. The function
   * only considers gcells used for accessing the pin in `traverseGraph`.
   *
   * @param rects A vector of guide rectangles.
   * @param pin_gcell_map A map of pins and their corresponding GCell
   * locations.
   * @param pins A vector of pins involved in the routing.
   * @return A vector of GCell locations for each pin (vector index is the pin
   * index in the adj_list_).
   */
  std::vector<std::vector<Point3D>> getPinToGCellList(
      const std::vector<frRect>& rects,
      const frBlockObjectMap<std::set<Point3D>>& pin_gcell_map,
      const std::vector<frBlockObject*>& pins) const;
  /**
   * @brief Updates the node map with pins and guides.
   *
   * The function updates the node_map_ with the guides and pin locations only
   * that were visited by `traverseGraph` and disregard unvisited guides and
   * pin gcells.
   *
   * @param rects A vector of guide rectangles.
   * @param pin_to_gcell A vector mapping pins to GCell locations.
   */
  void updateNodeMap(const std::vector<frRect>& rects,
                     const std::vector<std::vector<Point3D>>& pin_to_gcell);
  /**
   * @brief Returns vector of pins with their GCell locations.
   *
   * This function takes the computed pin-to-GCell mappings and updates the
   * corresponding gr pins with their respective GCell locations.
   *
   * @param pins A vector of block objects representing the pins.
   * @param pin_to_gcell A vector mapping pins to their GCell locations.
   * @returns A vector of pin-to-gcell pairs to be updated.
   */
  std::vector<std::pair<frBlockObject*, odb::Point>> getGRPins(
      const std::vector<frBlockObject*>& pins,
      const std::vector<std::vector<Point3D>>& pin_to_gcell) const;
  /**
   * @brief Does a bfs search from the given node idx.
   */
  void bfs(int node_idx);
  /**
   * @brief Clips guide endpoints that are going nowhere
   */
  void clipGuides(std::vector<frRect>& rects);
  /**
   * @brief Merges guides on same layer and same track
   */
  void mergeGuides(std::vector<frRect>& rects);
  frDesign* getDesign() const { return design_; }
  frTechObject* getTech() const { return design_->getTech(); }

  frDesign* design_{nullptr};
  utl::Logger* logger_{nullptr};
  RouterConfiguration* router_cfg_{nullptr};
  frNet* net_{nullptr};
  bool force_feed_through_{false};
  std::map<Point3D, std::set<int>> node_map_;
  int guide_count_{0};
  int node_count_{0};
  bool allow_warnings_{false};
  std::vector<std::vector<int>> adj_list_;
  std::vector<bool> visited_;
  std::vector<bool> is_on_path_;
  std::vector<int> prev_idx_;
  frBlockObjectMap<std::set<Point3D>> pin_gcell_map_;
  std::vector<frRect> rects_;
};
}  // namespace drt::io
