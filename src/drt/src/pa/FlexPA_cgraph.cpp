// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "pa/FlexPA_cgraph.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "db/infra/frTime.h"
#include "db/obj/frAccess.h"
#include "db/obj/frBlockObject.h"
#include "db/obj/frInst.h"
#include "db/obj/frInstTerm.h"
#include "db/obj/frVia.h"
#include "distributed/PinAccessJobDescription.h"
#include "distributed/frArchive.h"
#include "dst/Distributed.h"
#include "dst/JobMessage.h"
#include "frProfileTask.h"
#include "frRTree.h"
#include "gc/FlexGC.h"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"
#include "omp.h"
#include "pa/FlexPA.h"
#include "pa/FlexPA_unique.h"
#include "serialization.h"
#include "utl/Logger.h"
#include "utl/exception.h"

using namespace drt;

std::unique_ptr<frVia> FlexPA::buildVia(
    frInstTerm* iterm,
    frAccessPoint* ap,
    std::vector<std::pair<frConnFig*, frBlockObject*>>& objs)
{
  odb::dbTransform xform = iterm->getInst()->getNoRotationTransform();

  if (ap->hasAccess(frDirEnum::U)) {
    odb::Point pt1(ap->getPoint());
    xform.apply(pt1);
    std::unique_ptr<frVia> via1 = std::make_unique<frVia>(ap->getViaDef(), pt1);
    via1->setOrigin(pt1);
    if (iterm->hasNet()) {
      objs.emplace_back(via1.get(), iterm->getNet());
    } else {
      objs.emplace_back(via1.get(), iterm);
    }

    return via1;
  }

  return nullptr;
}

odb::Rect FlexPA::getViaBBox(frInstTerm* iterm, frAccessPoint* ap)
{
  odb::dbTransform xform = iterm->getInst()->getNoRotationTransform();
  odb::Point pt1(ap->getPoint());
  xform.apply(pt1);
  std::unique_ptr<frVia> via1 = std::make_unique<frVia>(ap->getViaDef(), pt1);
  via1->setOrigin(pt1);
  return via1->getBBox();
}

bool FlexPA::checkAPVio(frInstTerm* itermA,
                        frAccessPoint* ap_A,
                        frInstTerm* itermB,
                        frAccessPoint* ap_B)
{
  bool has_vio = false;

  std::vector<std::pair<frConnFig*, frBlockObject*>> objs;

  auto via1 = buildVia(itermA, ap_A, objs);
  auto via2 = buildVia(itermB, ap_B, objs);
  has_vio = !genPatternsGC({itermA->getInst(), itermB->getInst()}, objs, Edge);

  return has_vio;
}

std::unordered_map<UniqueClass*, std::vector<cgIntraCellEdge>>
FlexPA::genCGraphUniqueIntraCell()
{
  std::unordered_map<UniqueClass*, std::vector<cgIntraCellEdge>> intra_cells;

  std::cout << fmt::format("Computing intra edges for {} unique instances...",
                           unique_insts_.getUniqueClasses().size());
#pragma omp parallel for schedule(dynamic)
  for (auto& unique : unique_insts_.getUniqueClasses()) {
    frInst* unique_inst = unique->getFirstInst();
    int idx = unique->getPinAccessIdx();
    std::vector<std::pair<frInstTerm*, frMPin*>> pins;
    std::vector<cgIntraCellEdge> edges;

    for (auto& iterm : unique_inst->getInstTerms()) {
      for (auto& pin : iterm->getTerm()->getPins()) {
        pins.emplace_back(iterm.get(), pin.get());
      }
    }

    for (int i = 0; i < pins.size(); i++) {
      for (int j = i; j < pins.size(); j++) {
        auto term_i = pins[i].first;
        auto pin_i = pins[i].second;
        auto term_j = pins[j].first;
        auto pin_j = pins[j].second;
        const std::vector<std::unique_ptr<frAccessPoint>>& aps_i
            = pin_i->getPinAccess(idx)->getAccessPoints();
        const std::vector<std::unique_ptr<frAccessPoint>>& aps_j
            = pin_j->getPinAccess(idx)->getAccessPoints();

        // All APs on the same term conflict with each other
        if (term_i == term_j) {
          // All-to-all with deduplication if it is the same pin of the same
          // term
          if (pin_i == pin_j) {
            for (int a = 0; a < aps_i.size(); a++) {
              for (int b = a + 1; b < aps_i.size(); b++) {
                edges.emplace_back(term_i->getTerm(),
                                   aps_i[a].get(),
                                   term_i->getTerm(),
                                   aps_i[b].get());
              }
            }
            // All-to-all without deduplication if it is two separate pins of
            // same term
          } else {
            for (int a = 0; a < aps_i.size(); a++) {
              for (int b = 0; b < aps_j.size(); b++) {
                edges.emplace_back(term_i->getTerm(),
                                   aps_i[a].get(),
                                   term_i->getTerm(),
                                   aps_j[b].get());
              }
            }
          }
        } else {  // If we have two separate terms, then perform a
                  // violation check
          for (int a = 0; a < aps_i.size(); a++) {
            for (int b = 0; b < aps_j.size(); b++) {
              if (checkAPVio(term_i, aps_i[a].get(), term_j, aps_j[b].get())) {
                edges.emplace_back(term_i->getTerm(),
                                   aps_i[a].get(),
                                   term_j->getTerm(),
                                   aps_j[b].get());
              }
            }
          }
        }
      }
    }

#pragma omp critical
    intra_cells.emplace(unique.get(), edges);
  }

  std::cout << " Done\n";

  return intra_cells;
}

std::vector<cgCellEdge> FlexPA::genCGraphInflateIntraCell(
    std::vector<frInst*>& insts,
    std::unordered_map<UniqueClass*, std::vector<cgIntraCellEdge>>& uniques)
{
  std::vector<cgCellEdge> edges;

  for (frInst* inst : insts) {
    auto key = unique_insts_.computeUniqueClass(inst);
    std::vector<cgIntraCellEdge> intras = uniques[key];

    for (cgIntraCellEdge intra : intras) {
      frInstTerm* iterm0 = inst->getInstTerm(intra.term0->getIndexInOwner());
      frInstTerm* iterm1 = inst->getInstTerm(intra.term1->getIndexInOwner());
      edges.emplace_back(iterm0, intra.pa0, iterm1, intra.pa1);
    }
  }

  return edges;
}

using Access = std::pair<frInstTerm*, frPinAccess*>;

std::vector<cgCellEdge> FlexPA::genCGraphInterCell(
    std::vector<frInst*>& insts,
    bool window_level_parallelism)
{
  std::vector<cgCellEdge> edges;
  // std::cout << fmt::format("Start inter-cell for {} insts... ",
  // insts.size());

  // Build array of all possible access sets.
  std::vector<Access> accesses;
  for (auto inst : insts) {
    int idx = inst->getPinAccessIdx();
    for (auto& iterm : inst->getInstTerms()) {
      for (auto& pin : iterm->getTerm()->getPins()) {
        accesses.emplace_back(iterm.get(), pin->getPinAccess(idx));
      }
    }
  }
  // std::cout << fmt::format("Built access table with {} terms.\n",
  // accesses.size());

  // Build an rtree for searching for nearby terms.
  RTree<uint32_t> term_tree;
  for (uint32_t i = 0; i < accesses.size(); i++) {
    Access acc = accesses[i];
    odb::Rect bbox = acc.first->getBBox();
    odb::Rect fat_box;
    bbox.bloat(56, fat_box);
    term_tree.insert(std::make_pair(fat_box, i));
  }
  // std::cout << fmt::format("Built rtree.\n", insts.size());

  /* Iterate through all iterms and find all *unique* intersections of iterms
  We do this with an unordered map M(K,V) and we create an entry such that
  given two pins i, j, the key K represents the existence of (i, j) and (j,
  i). K = bitconcat(min(i,j), max(i, j)) Here, i and j represent the index
  in the accesses vector. V is an array of 2 Accesses. We can then iterate
  all entries of this map....
  */

  std::unordered_map<uint64_t, std::array<Access, 2>> neighbor_terms;

  for (uint32_t i1 = 0; i1 < accesses.size(); i1++) {
    Access acc1 = accesses[i1];
    frInstTerm* iterm = acc1.first;
    odb::Rect bbox = iterm->getBBox();
    for (auto it = term_tree.qbegin(bgi::intersects(bbox));
         it != term_tree.qend();
         ++it) {
      uint32_t i2 = it->second;
      if (i1 == i2) {
        continue;
      }
      Access acc2 = accesses[i2];
      uint64_t key
          = uint64_t(std::max(i1, i2)) << 32 | uint64_t(std::min(i1, i2));
      neighbor_terms[key] = {acc1, acc2};
    }
  }
  // std::cout << fmt::format("Built map of neighboring terms with {} edges.\n",
  // neighbor_terms.size());

  int e_cnt = 0;
  int e_cntTotal = 0;
  int n_cnt = 0;

  std::vector<std::pair<uint64_t, std::array<Access, 2>>> neighbor_terms_cache(
      neighbor_terms.begin(), neighbor_terms.end());

#pragma omp parallel if (!window_level_parallelism)
  {
    std::vector<cgCellEdge> edges_threaded;

#pragma omp for schedule(dynamic) nowait
    for (auto nterms : neighbor_terms_cache) {
      Access acc0 = nterms.second[0];
      Access acc1 = nterms.second[1];

      const std::vector<std::unique_ptr<frAccessPoint>>& aps0
          = acc0.second->getAccessPoints();
      const std::vector<std::unique_ptr<frAccessPoint>>& aps1
          = acc1.second->getAccessPoints();

      // This will range from 4-256 total iterations.
      //
      for (auto& ap0 : aps0) {
        for (auto& ap1 : aps1) {
          // #pragma omp atomic
          // e_cntTotal++;
          auto xform0 = acc0.first->getInst()->getNoRotationTransform();
          auto xform1 = acc1.first->getInst()->getNoRotationTransform();
          auto pt0 = ap0->getPoint();
          auto pt1 = ap1->getPoint();
          xform0.apply(pt0);
          xform1.apply(pt1);

          if (odb::Point::manhattanDistance(pt0, pt1) > 56) {
            continue;
          }
          // #pragma omp atomic
          // e_cnt++;
          if (checkAPVio(acc0.first, ap0.get(), acc1.first, ap1.get())) {
            edges_threaded.emplace_back(
                acc0.first, ap0.get(), acc1.first, ap1.get());
          }
        }
      }

#pragma omp atomic
      n_cnt++;

      if (!window_level_parallelism && n_cnt % 10000 == 0) {
#pragma omp critical
        logger_->info(
            DRT, 205, "Computed {} of {} Neighboring Term Pairs.", n_cnt, neighbor_terms_cache.size());
          }
        }
        
        #pragma omp critical
        edges.insert(edges.end(), edges_threaded.begin(), edges_threaded.end());
      }
      if(!window_level_parallelism) {
      logger_->info(
          DRT, 208, "Computed {} of {} Neighboring Term Pairs. Done.", n_cnt, neighbor_terms_cache.size());
      }
  /*std::cout << fmt::format(
      "Inter-cell done having generated {} edges after {} checks ({} "
      "skipped).\n",
      edges.size(),
      cnt,
      cntTotal - cnt);*/
  return edges;
}

cgConflictGraph FlexPA::genConflictGraphForWindow(
    std::string path,
    odb::Rect window,
    RTree<frInst*> insts_tree,
    std::unordered_map<UniqueClass*, std::vector<cgIntraCellEdge>>& intras,
    bool window_level_parallelism)
{
  std::vector<frInst*> insts;

  odb::Rect fat_window;
  window.bloat(56, fat_window);
  for (auto it = insts_tree.qbegin(bgi::intersects(fat_window));
       it != insts_tree.qend();
       ++it) {
    insts.push_back(it->second);
  }
  // std::cout << "For window: found window's insts." << std::endl;

  auto intra_edges = genCGraphInflateIntraCell(insts, intras);
  // std::cout << "Inflated Intra-cell edges" << std::endl;
  auto edges = genCGraphInterCell(insts, window_level_parallelism);
  edges.insert(edges.end(), intra_edges.begin(), intra_edges.end());

  return {path, window, edges};
}

std::vector<cgConflictGraph> FlexPA::genConflictGraphsHelper(
    std::vector<std::string> paths,
    std::vector<odb::Rect> windows,
    bool window_level_parallelism)
{
  RTree<frInst*> insts_tree;
  for (auto& inst_ptr : design_->getTopBlock()->getInsts()) {
    frInst* inst = inst_ptr.get();
    insts_tree.insert(std::make_pair(inst->getBBox(), inst));
  }

  omp_set_num_threads(router_cfg_->MAX_THREADS);

  auto intras = genCGraphUniqueIntraCell();

  std::vector<cgConflictGraph> cgraphs;
  int cnt = 0;

#pragma omp parallel if (window_level_parallelism)
  {
    std::vector<cgConflictGraph> cgraphs_threaded;

#pragma omp for schedule(dynamic) nowait
    for (int i = 0; i < windows.size(); i++) {
      cgConflictGraph cgraph = genConflictGraphForWindow(
          paths[i], windows[i], insts_tree, intras, window_level_parallelism);
      cgraphs_threaded.push_back(cgraph);

#pragma omp atomic
      cnt++;

      if ((cnt % 1000) == 0) {
#pragma omp critical
        logger_->info(
            DRT, 80, "Computed {} of {} Conflict Graphs.", cnt, windows.size());
      }
    }

#pragma omp critical
    cgraphs.insert(
        cgraphs.end(), cgraphs_threaded.begin(), cgraphs_threaded.end());
  }

  logger_->info(DRT,
                159,
                "Computed {} of {} Conflict Graphs. Done.",
                cnt,
                windows.size());
  return cgraphs;
}

void FlexPA::writeConflictGraphHelper(cgConflictGraph cgraph)
{
  std::ofstream csv;
  csv.open(cgraph.path);
  csv << "iterm1,iterm2,apA_x,apA_y,apB_x,apB_y,cost" << std::endl;
  for (cgCellEdge edge : cgraph.edges) {
    csv << fmt::format("{},{},{},{},{},{},{}\n",
                       edge.term0->getName(),
                       edge.term1->getName(),
                       edge.pa0->getPoint().x(),
                       edge.pa0->getPoint().y(),
                       edge.pa1->getPoint().x(),
                       edge.pa1->getPoint().y(),
                       1000);
  }
  csv.close();
}

void FlexPA::genConflictGraphs(const std::vector<std::string>& paths,
                               const std::vector<odb::Rect>& windows,
                               bool window_level_parallelism)
{
  init();
  genAllAccessPoints();
  revertAccessPoints();

  logger_->info(DRT,
                188,
                "Computing Conflict Graphs now for {} windows...",
                windows.size());
  std::vector<cgConflictGraph> cgraphs
      = genConflictGraphsHelper(paths, windows, window_level_parallelism);
  logger_->info(DRT,
                189,
                "Writing Conflict Graphs now for {} windows...",
                cgraphs.size());
  int cnt = 0;

#pragma omp parallel for schedule(dynamic)
  for (const auto& cgraph : cgraphs) {
    writeConflictGraphHelper(cgraph);

#pragma omp critical
    {
      cnt++;
      if (cnt % (cnt > 1000 ? 100 : 10) == 0) {
        logger_->info(
            DRT, 196, "  Wrote {} of {} Conflict Graphs.", cnt, windows.size());
      }
    }
  }
  logger_->info(
      DRT, 197, "Wrote {} of {} Conflict Graphs. Done.", cnt, windows.size());
}