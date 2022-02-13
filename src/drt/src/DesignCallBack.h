#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
namespace triton_route {
class TritonRoute;
}
namespace fr {
class DesignCallBack : public odb::dbBlockCallBackObj
{
 public:
  DesignCallBack(triton_route::TritonRoute* router) : router_(router) {}
  ~DesignCallBack() {}
  void inDbPostMoveInst(odb::dbInst* inst) override;

 private:
  triton_route::TritonRoute* router_;
};
}  // namespace fr
