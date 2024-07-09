#include "../../src/hier_rtlmp.h"
#include "gtest/gtest.h"
#include "helper.h"
#include "mpl2/rtl_mp.h"
#include "odb/db.h"
#include "odb/util.h"
#include "utl/Logger.h"

namespace mpl2 {
class Mpl2PusherTest : public ::testing::Test
{
 protected:
  template <class T>
  using OdbUniquePtr = std::unique_ptr<T, void (*)(T*)>;

  void SetUp() override
  {
    db_ = OdbUniquePtr<odb::dbDatabase>(odb::dbDatabase::create(),
                                        &odb::dbDatabase::destroy);
    chip_ = OdbUniquePtr<odb::dbChip>(odb::dbChip::create(db_.get()),
                                      &odb::dbChip::destroy);
    block_
        = OdbUniquePtr<odb::dbBlock>(chip_->getBlock(), &odb::dbBlock::destroy);
  }

  utl::Logger logger_;
  OdbUniquePtr<odb::dbDatabase> db_{nullptr, &odb::dbDatabase::destroy};
  OdbUniquePtr<odb::dbLib> lib_{nullptr, &odb::dbLib::destroy};
  OdbUniquePtr<odb::dbChip> chip_{nullptr, &odb::dbChip::destroy};
  OdbUniquePtr<odb::dbBlock> block_{nullptr, &odb::dbBlock::destroy};
};

// The ConstructPusher tests whether a Pusher object can be constructed
// without halting, and indirectly tests private function Pusher::SetIOBlockages. 
// 
// There are several cases based on the cluster type (see mpl2/object.h):
// 1. HardMacroCluster (ConstructPusherHardMacro)
//     -> Cluster only has leaf_macros_
// 2. StdCellCluster   (ConstructPusherStdCell)
//     -> Cluster only has leaf_std_cells_ and dbModules_
// 3. MixedCluster     (ConstructPusherMixed)     
//     -> Cluster has both std cells and hard macros

TEST_F(Mpl2PusherTest, ConstructPusherHardMacro)
{
  // Test whether a Cluster of type HardMacroCluster can be created
  // and then used to construct a Pusher object, and then whether the 
  // boundary_to_io_blockage_ created during construction has the expected
  // value (empty).

  utl::Logger* logger_ = new utl::Logger();
  odb::dbDatabase* db_ = createSimpleDB();
  db_->setLogger(logger_);

  odb::dbMaster* master_ = createSimpleMaster(
      db_->findLib("lib"), "simple_master", 1000, 1000, odb::dbMasterType::CORE);

  odb::dbBlock* block_ = odb::dbBlock::create(db_->getChip(), "simple_block");
  block_->setDieArea(odb::Rect(0, 0, 1000, 1000));

  odb::dbInst* inst1 = odb::dbInst::create(block_, master_, "leaf_macro");

  // Add dbNets, BTerms in order to indirectly test Pusher::setIOBlockages
  // (called during Pusher construction) later. 

  //              (inst1)
  // (n1)-------[in - out]--------(n2)
  //

  odb::dbNet* n1 = odb::dbNet::create(block_, "n1");
  odb::dbNet* n2 = odb::dbNet::create(block_, "n2");
  inst1->findITerm("in" )->connect(n1);
  inst1->findITerm("out")->connect(n2);

  odb::dbBTerm* IN = odb::dbBTerm::create(n1, "IN");
  IN->setIoType(odb::dbIoType::INPUT);
  odb::dbBTerm* OUT = odb::dbBTerm::create(n2, "OUT");
  OUT->setIoType(odb::dbIoType::OUTPUT);
  
  // Create cluster of type HardMacroCluster, and add one instance
  Cluster* cluster_ = new Cluster(0, std::string("hard_macro_cluster"), logger_);
  cluster_->setClusterType(HardMacroCluster);
  cluster_->addLeafMacro(inst1); 

  // In hier_rtlmp.cpp the io blockages would have been retrieved by
  // setIOClustersBlockages (-> computeIOSpans, computeIOBlockagesDepth)

  // In this case, the following results would be received:
  // io_spans[L, T, R, B].first = 1.0
  // io_spans[L, T, R, B].second = 0.0

  /*
  std::map<Boundary, std::pair<float, float>> io_spans;
  io_spans[L].first = 1.0;
  io_spans[L].second = 0.0;
  io_spans[T].first = 1.0;
  io_spans[T].second = 0.0;
  io_spans[R].first = 1.0;
  io_spans[R].second = 0.0;
  io_spans[B].first = 1.0;
  io_spans[B].second = 0.0;
  */

  /*
  odb::Rect die = block_->getDieArea();
  io_spans[L] = {
    block_->dbuToMicrons(die.yMax()), block_->dbuToMicrons(die.yMin())};
  io_spans[T] = {
    block_->dbuToMicrons(die.xMax()), block_->dbuToMicrons(die.xMin())};
  io_spans[R] = io_spans[L];
  io_spans[B] = io_spans[T];

  for (auto term : block_->getBTerms()) {
    int lx = std::numeric_limits<int>::max();
    int ly = std::numeric_limits<int>::max();
    int ux = 0;
    int uy = 0;

    for (const auto pin : term->getBPins()) {
      for (const auto box : pin->getBoxes()) {
        lx = std::min(lx, box->xMin());
        ly = std::min(ly, box->yMin());
        ux = std::max(ux, box->xMax());
        uy = std::max(uy, box->yMax());
      }
    }

    // Modify ranges based on the position of the IO pins.
    if (lx <= die.xMin()) {
      io_spans[L].first = std::min(
          io_spans[L].first, static_cast<float>(block_->dbuToMicrons(ly)));
      io_spans[L].second = std::max(
          io_spans[L].second, static_cast<float>(block_->dbuToMicrons(uy)));
    } else if (uy >= die.yMax()) {
      io_spans[T].first = std::min(
          io_spans[T].first, static_cast<float>(block_->dbuToMicrons(lx)));
      io_spans[T].second = std::max(
          io_spans[T].second, static_cast<float>(block_->dbuToMicrons(ux)));
    } else if (ux >= die.xMax()) {
      io_spans[R].first = std::min(
          io_spans[R].first, static_cast<float>(block_->dbuToMicrons(ly)));
      io_spans[R].second = std::max(
          io_spans[R].second, static_cast<float>(block_->dbuToMicrons(uy)));
    } else {
      io_spans[B].first = std::min(
          io_spans[B].first, static_cast<float>(block_->dbuToMicrons(lx)));
      io_spans[B].second = std::max(
          io_spans[B].second, static_cast<float>(block_->dbuToMicrons(ux)));
    }
  }

  logger_->report(
    "io_spans[L]: {} {},\nio_spans[T]: {} {},\nio_spans[R]: {} {},\nio_spans[B]: {} {},\n",
    io_spans[L].first,
    io_spans[L].second,
    io_spans[T].first,
    io_spans[T].second,
    io_spans[R].first,
    io_spans[R].second,
    io_spans[B].first,
    io_spans[B].second
  );
  */

  // IO blockages depth is computed by diving std_cell_area by sum_length
  // for Cluster of HardMacroCluster type, std_cell_area = 0, so depth = 0
  //const float depth = 0;
  //std::map<Boundary, Rect> boundary_to_io_blockage_;

  // Left, top, right, and bottom blockages are computed the second
  // part of each io_span is bigger than the first part, however 
  // in this case this is always untrue (always first > second)
  // so boundary_to_io_blockage_.size() will still be empty at the end.

  /*
  const Rect root(cluster_->getX(),
                  cluster_->getY(),
                  cluster_->getX() + cluster_->getWidth(),
                  cluster_->getY() + cluster_->getHeight());
  std::vector<Rect> macro_blockages_;
  if (io_spans[L].second > io_spans[L].first) {
    const Rect left_io_blockage(root.xMin(),
                                io_spans[L].first,
                                root.xMin() + depth,
                                io_spans[L].second);

    boundary_to_io_blockage_[L] = left_io_blockage;
    macro_blockages_.push_back(left_io_blockage);
  }

  if (io_spans[T].second > io_spans[T].first) {
    const Rect top_io_blockage(io_spans[T].first,
                               root.yMax() - depth,
                               io_spans[T].second,
                               root.yMax());

    boundary_to_io_blockage_[T] = top_io_blockage;
    macro_blockages_.push_back(top_io_blockage);
  }

  if (io_spans[R].second > io_spans[R].first) {
    const Rect right_io_blockage(root.xMax() - depth,
                                 io_spans[R].first,
                                 root.xMax(),
                                 io_spans[R].second);

    boundary_to_io_blockage_[R] = right_io_blockage;
    macro_blockages_.push_back(right_io_blockage);
  }

  if (io_spans[B].second > io_spans[B].first) {
    const Rect bottom_io_blockage(io_spans[B].first,
                                  root.yMin(),
                                  io_spans[B].second,
                                  root.yMin() + depth);

    boundary_to_io_blockage_[B] = bottom_io_blockage;
    macro_blockages_.push_back(bottom_io_blockage);
  }
  */

  //logger_->report("size of boundary_to_io_blockage_: {}", boundary_to_io_blockage_.size());

  // Construct Pusher object
  std::map<Boundary, Rect> boundary_to_io_blockage_;
  Pusher pusher(logger_, cluster_, block_, boundary_to_io_blockage_);
  
  // Due to getX, getY, getWidth, getHeight all being 0 due to Cluster type
  // (as defined in mpl2/object.cpp), 
  // it is expected that boundary_to_io_blockage_ here will be empty.
  EXPECT_TRUE(boundary_to_io_blockage_.size() == 0);
  
}  // ConstructPusherHardMacro

TEST_F(Mpl2PusherTest, ConstructPusherStdCell)
{
  // Test whether a Cluster of type StdCellCluster can be created
  // and then used to construct a Pusher object, and then whether the 
  // boundary_to_io_blockage_ created during construction has the expected
  // values.

  utl::Logger* logger_ = new utl::Logger();
  odb::dbDatabase* db_ = createSimpleDB();
  db_->setLogger(logger_);

  odb::dbMaster* master_ = createSimpleMaster(
      db_->findLib("lib"), "simple_master", 1000, 1000, odb::dbMasterType::CORE);

  odb::dbBlock* block_ = odb::dbBlock::create(db_->getChip(), "simple_block");
  block_->setDieArea(odb::Rect(0, 0, 1000, 1000));

  odb::dbInst::create(block_, master_, "leaf_std_cell1");
  odb::dbInst::create(block_, master_, "leaf_std_cell2");
  odb::dbInst::create(block_, master_, "leaf_std_cell3");

  Cluster* cluster_ = new Cluster(0, std::string("stdcell_cluster"), logger_);
  cluster_->setClusterType(StdCellCluster);
  cluster_->addDbModule(block_->getTopModule());

  // add declared instances as leaf std cells to cluster
  // and compute metrics alongside
  
  Metrics* metrics_ = new Metrics(0, 0, 0.0, 0.0);
  for (auto inst : block_->getInsts()) {

    const float inst_width = block_->dbuToMicrons(
        inst->getBBox()->getBox().dx());
    const float inst_height = block_->dbuToMicrons(
        inst->getBBox()->getBox().dy());
    
    cluster_->addLeafStdCell(inst);
    metrics_->addMetrics(Metrics(1, 0, inst_width * inst_height, 0.0));

  }

  cluster_->setMetrics(Metrics(
      metrics_->getNumStdCell(),
      metrics_->getNumMacro(),
      metrics_->getStdCellArea(), 
      metrics_->getMacroArea()
  ));

  cluster_->printBasicInformation(logger_);
  std::map<Boundary, Rect> boundary_to_io_blockage_;
  Pusher pusher(logger_, cluster_, block_, boundary_to_io_blockage_);

  logger_->report("area: {}", cluster_->getArea());

}  // ConstructPusherStdCell

}  // namespace mpl2
