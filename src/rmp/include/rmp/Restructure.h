// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <fstream>
#include <functional>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/Scene.hh"
#include "utl/Logger.h"
#include "utl/unique_name.h"

namespace abc {
}  // namespace abc

namespace odb {
class dbDatabase;
class dbBlock;
class dbInst;
class dbNet;
class dbITerm;
}  // namespace odb

namespace est {
class EstimateParasitics;
}

namespace sta {
class dbSta;
}  // namespace sta

namespace rmp {

enum class Mode
{
  AREA_1 = 0,
  AREA_2,
  AREA_3,
  DELAY_1,
  DELAY_2,
  DELAY_3,
  DELAY_4
};

class Restructure
{
 public:
  Restructure(utl::Logger* logger,
              sta::dbSta* open_sta,
              odb::dbDatabase* db,
              rsz::Resizer* resizer,
              est::EstimateParasitics* estimate_parasitics);
  ~Restructure();

  void reset();
  void resynth(sta::Scene* corner);
  void resynthAnnealing(sta::Scene* corner);
  void resynthGenetic(sta::Scene* corner);
  void run(char* liberty_file_name,
           float slack_threshold,
           unsigned max_depth,
           char* workdir_name,
           char* abc_logfile);

  void setAnnealingSeed(std::mt19937::result_type seed)
  {
    annealing_seed_ = seed;
  }
  void setAnnealingTemp(float temp) { annealing_temp_ = temp; }
  void setAnnealingIters(unsigned iters) { annealing_iters_ = iters; }
  void setAnnealingRevertAfter(unsigned revert_after)
  {
    annealing_revert_after_ = revert_after;
  }
  void setAnnealingInitialOps(unsigned ops) { annealing_init_ops_ = ops; }
  void setGeneticSeed(std::mt19937::result_type seed) { genetic_seed_ = seed; }
  void setGeneticPopulationSize(unsigned population_size)
  {
    genetic_population_size_ = population_size;
  }
  void setGeneticMutationProbability(float mutation_probability)
  {
    genetic_mutation_probability_ = mutation_probability;
  }
  void setGeneticCrossoverProbability(float crossover_probability)
  {
    genetic_crossover_probability_ = crossover_probability;
  }
  void setGeneticTournamentSize(unsigned tournament_size)
  {
    genetic_tournament_size_ = tournament_size;
  }
  void setGeneticTournamentProbability(float tournament_probability)
  {
    genetic_tournament_probability_ = tournament_probability;
  }
  void setGeneticIters(unsigned iters) { genetic_iters_ = iters; }
  void setGeneticInitialOps(unsigned ops) { genetic_init_ops_ = ops; }
  void setSlackThreshold(sta::Slack thresh) { slack_threshold_ = thresh; }
  void setMode(const char* mode_name);
  void setTieLoPort(sta::LibertyPort* loport);
  void setTieHiPort(sta::LibertyPort* hiport);

 private:
  void deleteComponents();
  void getBlob(unsigned max_depth);
  void runABC();
  void postABC(float worst_slack);
  bool writeAbcScript(const std::string& file_name);
  void writeOptCommands(std::ofstream& script);
  void initDB();
  void getEndPoints(sta::PinSet& ends, bool area_mode, unsigned max_depth);
  int countConsts(odb::dbBlock* top_block);
  void removeConstCells();
  void removeConstCell(odb::dbInst* inst);
  bool readAbcLog(const std::string& abc_file_name,
                  int& level_gain,
                  float& delay_val);

  utl::Logger* logger_;
  utl::UniqueName name_generator_;
  std::string logfile_;
  std::string locell_;
  std::string loport_;
  std::string hicell_;
  std::string hiport_;
  std::string work_dir_name_;

  // db vars
  sta::dbSta* open_sta_;
  odb::dbDatabase* db_;
  rsz::Resizer* resizer_;
  est::EstimateParasitics* estimate_parasitics_;
  odb::dbBlock* block_ = nullptr;

  // Annealing
  std::mt19937::result_type annealing_seed_ = 0;
  std::optional<float> annealing_temp_;
  unsigned annealing_iters_ = 100;
  std::optional<unsigned> annealing_revert_after_;
  unsigned annealing_init_ops_ = 10;

  // Genetic
  std::mt19937::result_type genetic_seed_ = 0;
  unsigned genetic_population_size_ = 4;
  float genetic_mutation_probability_ = 0.5;
  float genetic_crossover_probability_ = 0.5;
  unsigned genetic_tournament_size_ = 4;
  float genetic_tournament_probability_ = 0.8;
  unsigned genetic_iters_ = 10;
  unsigned genetic_init_ops_ = 10;

  sta::Slack slack_threshold_ = 0;

  std::string input_blif_file_name_;
  std::string output_blif_file_name_;
  std::vector<std::string> lib_file_names_;
  std::set<odb::dbInst*> path_insts_;

  Mode opt_mode_{Mode::DELAY_1};
  bool is_area_mode_{false};
  int blif_call_id_{0};
};

}  // namespace rmp
