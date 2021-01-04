///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "ZObject.h"
#include "odb.h"
#include "geom.h"

namespace odb {

///
/// IZdcr
///
///

class IZdcr : public ZObject
{
 public:
  ZINTERFACE_ID(IZdcr);

  virtual uint getAppGuiCnt() = 0;
  virtual void inspect()      = 0;
  virtual int  inspect(const char* objectName,
                       const char* selectType,
                       const char* action,
                       const char* chip,
                       const char* bb,
                       const char* layer,
                       const char* hier,
                       const char* type,
                       FILE*       outFP,
                       bool        noclip,
                       bool        write2list)
      = 0;

  virtual void chip_get(const char* chip,
                        bool        gridFlag,
                        int         maxobjects,
                        FILE*       outFP)
      = 0;
  virtual void chipAttr(char* layers, uint n, bool def) = 0;

  virtual void init(ZContext&   context,
                    bool        dbEnv,
                    const char* name,
                    const char* module,
                    bool        noStore)
      = 0;
  virtual char* setName(const char* name) = 0;
  virtual char* getName()                 = 0;
  //	virtual Ath__zui *getZui()=0;
  virtual void setBlockId(uint v)        = 0;
  virtual uint getBlockId()              = 0;
  virtual bool isBlockSelected(uint bid) = 0;

  virtual char* getInspectName() = 0;

  virtual uint addSubMenu(uint menuId, const char* name, uint boxId) = 0;
  virtual uint getMenuId(const char* name)                           = 0;
  virtual uint initDbMenus()                                         = 0;
  virtual uint setupModuleMenu(const char* moduleName,
                               const char* onechar,
                               uint        id)
      = 0;
  virtual void getChipAttr(char* bbox,
                           char* layers,
                           char* menus,
                           char* blocks,
                           char* defTypes)
      = 0;
  virtual uint getMenuNames(char* buff)                       = 0;
  virtual void writeToTcl(uint lineLimit, FILE* fp, bool w2l) = 0;
  virtual uint setInspectEnv(const char* action,
                             const char* objectName,
                             const char* bb,
                             const char* selectType,
                             bool        noclip)
      = 0;
  virtual void setFilters(const char* chip,
                          const char* layer,
                          const char* type,
                          const char* hier)
      = 0;
  virtual bool  getPullDownMenu(const char* objectName)        = 0;
  virtual uint  getSubmenuObjId(uint* id2)                     = 0;
  virtual uint  getPullDownActionId()                          = 0;
  virtual bool  msgAction()                                    = 0;
  virtual bool  selectAction()                                 = 0;
  virtual bool  isSelectedMenu(const char* name)               = 0;
  virtual uint  getSelectIds(uint* id1, uint* id2, uint* id3)  = 0;
  virtual bool  isSubmenuType(uint id)                         = 0;
  virtual char* getMsgBuffer(bool setBuffFlag)                 = 0;
  virtual void  print_self(char* blk, char* name, char* extra) = 0;
  virtual void  wireMsg(int   x1,
                        int   y1,
                        int   x2,
                        int   y2,
                        uint  level,
                        char* msg_buf)
      = 0;

  virtual void  getBbox(int* x1, int* y1, int* x2, int* y2) = 0;
  virtual char* getFirstTclBox(int* x1, int* y1, int* x2, int* y2, uint* layer)
      = 0;

  virtual void resetSearchParams(char*       chip,
                                 const char* bb,
                                 const char* layer,
                                 const char* hier,
                                 const char* type,
                                 bool        noclip,
                                 bool        q)
      = 0;

  virtual int getCoords(const char* bb, int* x1, int* y1, int* x2, int* y2) = 0;

  virtual uint addBox(uint        id,
                      uint        subMenuId,
                      uint        menuId,
                      int         layer,
                      int         x1,
                      int         y1,
                      int         x2,
                      int         y2,
                      uint        ownId,
                      const char* boxType = NULL)
      = 0;
  virtual uint addArrow(bool    right,
                        uint    boxType,
                        uint    hier,
                        int     layer,
                        int     labelCnt,
                        char**  label,
                        double* val,
                        int     x1,
                        int     y1,
                        int     x2,
                        int     y2,
                        uint    boxFilter)
      = 0;
  virtual uint addBoxAndMsg(uint        id,
                            uint        subMenuId,
                            uint        menuId,
                            int         layer,
                            int         x1,
                            int         y1,
                            int         x2,
                            int         y2,
                            uint        ownId,
                            const char* special)
      = 0;

  virtual bool* getExcludeLayerTable()                      = 0;
  virtual bool  getSubMenuFlag(uint menuId, uint subMenuId) = 0;
  virtual uint  addPullDownMenu(uint        menuId,
                                uint        subMenuId,
                                const char* db_name,
                                const char* zui_action)
      = 0;
  virtual uint addPullDownMenu(const char* menu,
                               const char* subMenu,
                               const char* db_name,
                               const char* zui_action)
      = 0;
  virtual bool validSearchBbox()                             = 0;
  virtual void setSearchBox(int x1, int y1, int x2, int y2)  = 0;
  virtual bool clipBox(int& x1, int& y1, int& x2, int& y2)   = 0;
  virtual bool invalidateSearchBox()                         = 0;
  virtual bool isInspectMenu(uint menuId)                    = 0;
  virtual bool isInspectSubMenu(uint subMenuId)              = 0;
  virtual bool isInspectSubMenu(uint menuId, uint subMenuId) = 0;

  virtual void setContextMarker() = 0;
  virtual void setSignalMarker()  = 0;
  virtual void setInstMarker()    = 0;
  virtual void resetMarker()      = 0;

  virtual void* getDpt()     = 0;
  virtual uint  writeToDpt() = 0;
};

}  // namespace odb


