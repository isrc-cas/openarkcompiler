/*
 * Copyright (c) [2019] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 */
#ifndef MAPLE_ME_INCLUDE_VER_SYMBOL_H
#define MAPLE_ME_INCLUDE_VER_SYMBOL_H
#include <iostream>
#include "mir_module.h"
#include "mir_symbol.h"
#include "orig_symbol.h"

// This file defines the data structure VersionSt that represents an SSA version
struct BBId;

namespace maple {
class BB;  // circular dependency exists, no other choice
class PhiNode;  // circular dependency exists, no other choice
class MayDefNode;  // circular dependency exists, no other choice
class MustDefNode;  // circular dependency exists, no other choice
class VersionStTable;  // circular dependency exists, no other choice
class OriginalSt;  // circular dependency exists, no other choice
class VersionSt {
 public:
  enum DefType {
    kDassign,
    kRegassign,
    kPhi,
    kMayDef,
    kMustDef
  };

  VersionSt(size_t index, uint32 version, OriginalSt *ost)
      : index(index),
        version(version),
        ost(ost),
        defBB(nullptr),
        defType(kDassign),
        defStmt(),
        live(false),
        isReturn(false) {}

  ~VersionSt() = default;

  size_t GetIndex() const {
    return index;
  }

  int GetVersion() const {
    return version;
  }

  void SetDefBB(BB *defbb) {
    defBB = defbb;
  }

  const BB *GetDefBB() const {
    return defBB;
  }

  void DumpDefStmt(const MIRModule *mod) const;

  bool IsLive() const {
    return live;
  }

  bool IsInitVersion() const {
    return version == kInitVersion;
  }

  DefType GetDefType() const {
    return defType;
  }

  void SetDefType(DefType defType) {
    this->defType = defType;
  }

  void MarkLive() {
    live = true;
  }

  void MarkDead() {
    live = false;
  }

  OStIdx GetOrigIdx() const {
    return ost->GetIndex();
  }

  const OriginalSt *GetOrigSt() const {
    return ost;
  }
  OriginalSt *GetOrigSt() {
    return ost;
  }
  void SetOrigSt(OriginalSt *ost) {
    this->ost = ost;
  }

  const DassignNode *GetDassignNode() const {
    return defStmt.dassign;
  }
  DassignNode *GetDassignNode() {
    return defStmt.dassign;
  }
  void SetDassignNode(DassignNode *dassignNode) {
    defStmt.dassign = dassignNode;
  }

  RegassignNode *GetRegassignNode() {
    return defStmt.regassign;
  }
  const RegassignNode *GetRegassignNode() const {
    return defStmt.regassign;
  }
  void SetRegassignNode(RegassignNode *regAssignNode) {
    defStmt.regassign = regAssignNode;
  }

  const PhiNode *GetPhi() const {
    return defStmt.phi;
  }
  PhiNode *GetPhi() {
    return defStmt.phi;
  }
  void SetPhi(PhiNode *phiNode) {
    defStmt.phi = phiNode;
  }

  const MayDefNode *GetMayDef() const {
    return defStmt.mayDef;
  }
  MayDefNode *GetMayDef() {
    return defStmt.mayDef;
  }
  void SetMayDef(MayDefNode *mayDefNode) {
    defStmt.mayDef = mayDefNode;
  }

  const MustDefNode *GetMustDef() const {
    return defStmt.mustDef;
  }
  MustDefNode *GetMustDef() {
    return defStmt.mustDef;
  }
  void SetMustDef(MustDefNode *mustDefNode) {
    defStmt.mustDef = mustDefNode;
  }

  bool IsReturn() const {
    return isReturn;
  }

  void Dump(const MIRModule *mod, bool omitName = false) const {
    if (!omitName) {
      ost->Dump();
    }
    LogInfo::MapleLogger() << "(" << version << ")";
  }

  bool DefByMayDef() const {
    return defType == kMayDef;
  }

 private:
  size_t index;     // index number in versionst_table_
  int version;      // starts from 0 for each symbol
  OriginalSt *ost;  // the index of related originalst in originalst_table
  BB *defBB;
  DefType defType;

  union DefStmt {
    DassignNode *dassign;
    RegassignNode *regassign;
    PhiNode *phi;
    MayDefNode *mayDef;
    MustDefNode *mustDef;
  } defStmt;  // only valid after SSA

  bool live;      // helper of dse
  bool isReturn;  // the symbol will return in its function
};

class VersionStTable {
 public:
  explicit VersionStTable(MemPool *vstMp) : vstAlloc(vstMp), versionStVector(vstAlloc.Adapter()) {}

  ~VersionStTable() = default;

  VersionSt *CreateVersionSt(OriginalSt *ost, size_t version);
  VersionSt *FindOrCreateVersionSt(OriginalSt *ost, size_t version);
  VersionSt *GetVersionStFromID(size_t id, bool checkFirst = false) const {
    if (checkFirst && id >= versionStVector.size()) {
      return nullptr;
    }
    ASSERT(id < versionStVector.size(), "symbol table index out of range");
    return versionStVector[id];
  }

  VersionSt &GetDummyVersionSt() {
    return dummyVST;
  }

  VersionSt *CreateVSymbol(VersionSt *vst, size_t version) {
    OriginalSt *ost = vst->GetOrigSt();
    return CreateVersionSt(ost, version);
  }

  bool Verify() const {
    return true;
  }

  size_t GetVersionStVectorSize() const {
    return versionStVector.size();
  }

  VersionSt *GetVersionStVectorItem(size_t index) const {
    CHECK_FATAL(index < versionStVector.size(), "out of range");
    return versionStVector[index];
  }

  void SetVersionStVectorItem(size_t index, VersionSt *vst) {
    CHECK_FATAL(index < versionStVector.size(), "out of range");
    versionStVector[index] = vst;
  }

  MapleAllocator &GetVSTAlloc() {
    return vstAlloc;
  }

  void Dump(MIRModule *mod) const;

 private:
  MapleAllocator vstAlloc;                   // this stores versionStVector
  MapleVector<VersionSt*> versionStVector;   // the vector that map a versionst's index to its pointer
  static VersionSt dummyVST;
};
}       // namespace maple
#endif  // MAPLE_ME_INCLUDE_VER_SYMBOL_H
