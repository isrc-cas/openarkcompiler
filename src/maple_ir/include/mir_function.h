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
#ifndef MAPLE_IR_INCLUDE_MIR_FUNCTION_H
#define MAPLE_IR_INCLUDE_MIR_FUNCTION_H
#include <string>
#include "mir_module.h"
#include "mir_const.h"
#include "mir_symbol.h"
#include "mir_preg.h"
#include "intrinsics.h"
#include "global_tables.h"
#include "file_layout.h"
#include "mir_nodes.h"


namespace maple {
enum MIRFuncProp {
  kFuncPropNone,
  kFuncPropHasCall,      // the function has call
  kFuncPropRetStruct,    // the function returns struct
  kFuncPropUserFunc,     // the function is a user func
  kFuncPropInfoPrinted,  // to avoid printing framesize/moduleid/funcSize info more than once per function since they
  // can only be printed at the beginning of a block
  kFuncPropNeverReturn,  // the function when called never returns
};

#define FUNCHASCALL (1U << kFuncPropHasCall)
#define FUNCRETSTRUCT (1U << kFuncPropRetStruct)
#define FUNCUSER (1U << kFuncPropUserFunc)
#define FUNCINFOPRINTED (1U << kFuncPropInfoPrinted)
#define FUNCNEVERRETURN (1U << kFuncPropNeverReturn)
// mapping src (java) variable to mpl variables to display debug info
struct MIRAliasVars {
  GStrIdx memPoolStrIdx;
  TyIdx tyIdx;
  GStrIdx sigStrIdx;
};

constexpr uint8 kDefEffect = 0x80;
constexpr uint8 kUseEffect = 0x40;
constexpr uint8 kIpaSeen = 0x20;
constexpr uint8 kPureFunc = 0x10;
constexpr uint8 kNoDefArgEffect = 0x8;
constexpr uint8 kNoDefEffect = 0x4;
constexpr uint8 kNoRetNewlyAllocObj = 0x2;
constexpr uint8 kNoThrowException = 0x1;

class MeFunction;  // circular dependency exists, no other choice
class EAConnectionGraph;  // circular dependency exists, no other choice
class MIRFunction {
 public:
  MIRFunction(MIRModule *mod, const StIdx sidx)
      : module(mod),
        puIdx(0),
        symbolTableIdx(sidx),
        formals(mod->GetMPAllocator().Adapter()),
        retRefSym(mod->GetMPAllocator().Adapter()),
        argumentsTyIdx(mod->GetMPAllocator().Adapter()),
        argumentsAttrs(mod->GetMPAllocator().Adapter()),
        dataMemPool(memPoolCtrler.NewMemPool("func data mempool")),
        dataMPAllocator(dataMemPool),
        codeMemPool(memPoolCtrler.NewMemPool("func code mempool")),
        codeMemPoolAllocator(codeMemPool),
        info(mod->GetMPAllocator().Adapter()),
        infoIsString(mod->GetMPAllocator().Adapter()),
        aliasVarMap(std::less<GStrIdx>(), mod->GetMPAllocator().Adapter()),
        withLocInfo(true)
  {
    frameSize = 0;
    upFormalSize = 0;
    moduleID = 0;
    funcSize = 0;
    tempCount = 0;
    puIdxOrigin = 0;
    baseFuncStrIdx = GStrIdx(0);
    baseClassStrIdx = GStrIdx(0);
    baseFuncWithTypeStrIdx = GStrIdx(0);
    signatureStrIdx = GStrIdx(0);
    hashCode = 0;
    layoutType = kLayoutUnused;
    mefunc = nullptr;
    eacg = nullptr;
  }

  ~MIRFunction() = default;

  void Init() {
    // Initially allocate symTab and pregTab on the module mempool for storing
    // parameters. If later mirfunction turns out to be a definition, new
    // tables will be allocated on the local data mempool.
    symTab = module->GetMemPool()->New<MIRSymbolTable>(module->GetMPAllocator());
    pregTab = module->GetMemPool()->New<MIRPregTable>(module, &module->GetMPAllocator());
    typeNameTab = module->GetMemPool()->New<MIRTypeNameTable>(module->GetMPAllocator());
    labelTab = module->GetMemPool()->New<MIRLabelTable>(module->GetMPAllocator());
  }

  void Dump(bool withoutBody = false);
  void DumpUpFormal(int32 indent) const;
  void DumpFrame(int32 indent) const;
  void DumpFuncBody(int32 indent);
  MIRSymbol *GetFuncSymbol() const {
    return GlobalTables::GetGsymTable().GetSymbolFromStidx(symbolTableIdx.Idx());
  }

  void SetBaseClassFuncNames(GStrIdx stridx);
  void SetMemPool(MemPool *memPool) {
    SetCodeMemPool(memPool);
    codeMemPoolAllocator.SetMemPool(codeMemPool);
  }

  /// update signature_stridx, basefunc_stridx, baseclass_stridx, basefunc_withtype_stridx
  /// without considering baseclass_stridx, basefunc_stridx's original non-zero values
  /// \param stridx full_name stridx of the new function name
  void OverrideBaseClassFuncNames(GStrIdx stridx);
  const std::string &GetName() const {
    return GlobalTables::GetGsymTable().GetSymbolFromStidx(symbolTableIdx.Idx())->GetName();
  }

  GStrIdx GetNameStrIdx() const {
    return GlobalTables::GetGsymTable().GetSymbolFromStidx(symbolTableIdx.Idx())->GetNameStrIdx();
  }

  const std::string &GetBaseClassName() const {
    return GlobalTables::GetStrTable().GetStringFromStrIdx(baseClassStrIdx);
  }

  const std::string &GetBaseFuncName() const {
    return GlobalTables::GetStrTable().GetStringFromStrIdx(baseFuncStrIdx);
  }

  const std::string &GetBaseFuncNameWithType() const {
    return GlobalTables::GetStrTable().GetStringFromStrIdx(baseFuncWithTypeStrIdx);
  }


  const std::string &GetSignature() const {
    return GlobalTables::GetStrTable().GetStringFromStrIdx(signatureStrIdx);
  }

  GStrIdx GetBaseClassNameStrIdx() const {
    return baseClassStrIdx;
  }

  GStrIdx GetBaseFuncNameStrIdx() const {
    return baseFuncStrIdx;
  }

  GStrIdx GetBaseFuncNameWithTypeStrIdx() const {
    return baseFuncWithTypeStrIdx;
  }


  void SetBaseClassNameStrIdx(GStrIdx id) {
    baseClassStrIdx.SetIdx(id.GetIdx());
  }

  void SetBaseFuncNameStrIdx(GStrIdx id) {
    baseFuncStrIdx.SetIdx(id.GetIdx());
  }

  void SetBaseFuncNameWithTypeStrIdx(GStrIdx id) {
    baseFuncWithTypeStrIdx.SetIdx(id.GetIdx());
  }

  MIRType *GetReturnType() const {
    return GlobalTables::GetTypeTable().GetTypeFromTyIdx(funcType->GetRetTyIdx());
  }

  bool IsReturnVoid() const {
    return GetReturnType()->GetPrimType() == PTY_void;
  }

  TyIdx GetReturnTyIdx() const {
    return funcType->GetRetTyIdx();
  }

  MIRType *GetClassType() const {
    return GlobalTables::GetTypeTable().GetTypeFromTyIdx(classTyIdx);
  }

  TyIdx GetClassTyIdx() const {
    return classTyIdx;
  }
  void SetClassTyIdx(TyIdx tyIdx) {
    classTyIdx = tyIdx;
  }
  void SetClassTyIdx(uint32 idx) {
    classTyIdx.SetIdx(idx);
  }

  void SetReturnTyIdx(TyIdx tyidx) {
    funcType->SetRetTyIdx(tyidx);
  }

  size_t GetParamSize() const {
    return funcType->GetParamTypeList().size();
  }

  const std::vector<TyIdx> &GetParamTypes() const {
    return funcType->GetParamTypeList();
  }

  TyIdx GetNthParamTyIdx(size_t i) const {
    CHECK_FATAL(i < funcType->GetParamTypeList().size(), "array index out of range");
    return funcType->GetParamTypeList()[i];
  }

  MIRType *GetNthParamType(size_t i) const {
    CHECK_FATAL(i < funcType->GetParamTypeList().size(), "array index out of range");
    return GlobalTables::GetTypeTable().GetTypeFromTyIdx(funcType->GetParamTypeList()[i]);
  }

  const TypeAttrs &GetNthParamAttr(size_t i) const {
    CHECK_FATAL(i < funcType->GetParamAttrsList().size(), "array index out of range");
    return funcType->GetParamAttrsList()[i];
  }

  void SetNthParamAttr(size_t i, TypeAttrs ta) const {
    CHECK_FATAL(i < funcType->GetParamAttrsList().size(), "array index out of range");
    funcType->GetParamAttrsList()[i] = ta;
  }

  void AddArgument(MIRSymbol *st) {
    formals.push_back(st);
    funcType->GetParamTypeList().push_back(st->GetTyIdx());
    funcType->GetParamAttrsList().push_back(st->GetAttrs());
  }

  LabelIdx GetOrCreateLableIdxFromName(const std::string &name) {
    // TODO: this function should never be used after parsing, so move to parser?
    GStrIdx stridx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
    LabelIdx labidx = GetLabelTab()->GetStIdxFromStrIdx(stridx);
    if (labidx == 0) {
      labidx = GetLabelTab()->CreateLabel();
      GetLabelTab()->SetSymbolFromStIdx(labidx, stridx);
      GetLabelTab()->AddToStringLabelMap(labidx);
    }
    return labidx;
  }

  const GStrIdx GetLabelStringIndex(LabelIdx lbidx) const {
    CHECK(lbidx < labelTab->Size(), "index out of range in GetLabelStringIndex");
    return labelTab->GetSymbolFromStIdx(lbidx);
  }

  const std::string &GetLabelName(LabelIdx lbidx) const {
    GStrIdx stridx = GetLabelStringIndex(lbidx);
    return GlobalTables::GetStrTable().GetStringFromStrIdx(stridx);
  }

  MIRSymbol *GetLocalOrGlobalSymbol(const StIdx &idx, bool checkfirst = false) const;

  void SetAttrsFromSe(uint8 se) {
    // NoPrivateDefEffect
    if ((se & kDefEffect) == kDefEffect) {
      funcAttrs.SetAttr(FUNCATTR_noprivate_defeffect);
    }
    // NoPrivateUseEffect
    if ((se & kUseEffect) == kUseEffect) {
      funcAttrs.SetAttr(FUNCATTR_noprivate_useeffect);
    }
    // IpaSeen
    if ((se & kIpaSeen) == kIpaSeen) {
      funcAttrs.SetAttr(FUNCATTR_ipaseen);
    }
    // Pure
    if ((se & kPureFunc) == kPureFunc) {
      funcAttrs.SetAttr(FUNCATTR_pure);
    }
    // NoDefArgEffect
    if ((se & kNoDefArgEffect) == kNoDefArgEffect) {
      funcAttrs.SetAttr(FUNCATTR_nodefargeffect);
    }
    // NoDefEffect
    if ((se & kNoDefEffect) == kNoDefEffect) {
      funcAttrs.SetAttr(FUNCATTR_nodefeffect);
    }
    // NoRetNewlyAllocObj
    if ((se & kNoRetNewlyAllocObj) == kNoRetNewlyAllocObj) {
      funcAttrs.SetAttr(FUNCATTR_noret_newly_alloc_obj);
    }
    // NoThrowException
    if ((se & kNoThrowException) == kNoThrowException) {
      funcAttrs.SetAttr(FUNCATTR_nothrow_exception);
    }
  }
  bool GetAttr(FuncAttrKind x) const {
    return funcAttrs.GetAttr(x);
  }
  void SetAttr(FuncAttrKind x) {
    funcAttrs.SetAttr(x);
  }

  bool IsVarargs() const {
    return funcAttrs.GetAttr(FUNCATTR_varargs);
  }

  bool IsWeak() const {
    return funcAttrs.GetAttr(FUNCATTR_weak);
  }

  bool IsStatic() const {
    return funcAttrs.GetAttr(FUNCATTR_static);
  }

  bool IsNative() const {
    return funcAttrs.GetAttr(FUNCATTR_native);
  }

  bool IsFinal() const {
    return funcAttrs.GetAttr(FUNCATTR_final);
  }

  bool IsAbstract() const {
    return funcAttrs.GetAttr(FUNCATTR_abstract);
  }

  bool IsPublic() const {
    return funcAttrs.GetAttr(FUNCATTR_public);
  }

  bool IsPrivate() const {
    return funcAttrs.GetAttr(FUNCATTR_private);
  }

  bool IsProtected() const {
    return funcAttrs.GetAttr(FUNCATTR_protected);
  }

  bool IsPackagePrivate() const {
    return !IsPublic() && !IsPrivate() && !IsProtected();
  }

  bool IsConstructor() const {
    return funcAttrs.GetAttr(FUNCATTR_constructor);
  }

  bool IsLocal() const {
    return funcAttrs.GetAttr(FUNCATTR_local);
  }

  bool IsNoDefArgEffect() const {
    return funcAttrs.GetAttr(FUNCATTR_nodefargeffect);
  }

  bool IsNoDefEffect() const {
    return funcAttrs.GetAttr(FUNCATTR_nodefeffect);
  }

  bool IsNoRetNewlyAllocObj() const {
    return funcAttrs.GetAttr(FUNCATTR_noret_newly_alloc_obj);
  }

  bool IsNoThrowException() const {
    return funcAttrs.GetAttr(FUNCATTR_nothrow_exception);
  }

  bool IsNoPrivateUseEffect() const {
    return funcAttrs.GetAttr(FUNCATTR_noprivate_useeffect);
  }

  bool IsNoPrivateDefEffect() const {
    return funcAttrs.GetAttr(FUNCATTR_noprivate_defeffect);
  }

  bool IsIpaSeen() const {
    return funcAttrs.GetAttr(FUNCATTR_ipaseen);
  }

  bool IsPure() const {
    return funcAttrs.GetAttr(FUNCATTR_pure);
  }

  void SetVarArgs() {
    funcAttrs.SetAttr(FUNCATTR_varargs);
  }

  void SetNoDefArgEffect() {
    funcAttrs.SetAttr(FUNCATTR_nodefargeffect);
  }

  void SetNoDefEffect() {
    funcAttrs.SetAttr(FUNCATTR_nodefeffect);
  }

  void SetNoRetNewlyAllocObj() {
    funcAttrs.SetAttr(FUNCATTR_noret_newly_alloc_obj);
  }

  void SetNoThrowException() {
    funcAttrs.SetAttr(FUNCATTR_nothrow_exception);
  }

  void SetNoPrivateUseEffect() {
    funcAttrs.SetAttr(FUNCATTR_noprivate_useeffect);
  }

  void SetNoPrivateDefEffect() {
    funcAttrs.SetAttr(FUNCATTR_noprivate_defeffect);
  }

  void SetIpaSeen() {
    funcAttrs.SetAttr(FUNCATTR_ipaseen);
  }

  void SetPure() {
    funcAttrs.SetAttr(FUNCATTR_pure);
  }

  void UnsetNoDefArgEffect() {
    funcAttrs.SetAttr(FUNCATTR_nodefargeffect, true);
  }

  void UnsetNoDefEffect() {
    funcAttrs.SetAttr(FUNCATTR_nodefeffect, true);
  }

  void UnsetNoRetNewlyAllocObj() {
    funcAttrs.SetAttr(FUNCATTR_noret_newly_alloc_obj, true);
  }

  void UnsetNoThrowException() {
    funcAttrs.SetAttr(FUNCATTR_nothrow_exception, true);
  }

  void UnsetPure() {
    funcAttrs.SetAttr(FUNCATTR_pure, true);
  }

  void UnsetNoPrivateUseEffect() {
    funcAttrs.SetAttr(FUNCATTR_noprivate_useeffect, true);
  }

  void UnsetNoPrivateDefEffect() {
    funcAttrs.SetAttr(FUNCATTR_noprivate_defeffect, true);
  }

  bool HasCall() const {
    return flag & FUNCHASCALL;
  }

  void SetHasCall() {
    flag |= FUNCHASCALL;
  }

  bool IsReturnStruct() const {
    return flag & FUNCRETSTRUCT;
  }

  void SetReturnStruct() {
    flag |= FUNCRETSTRUCT;
  }

  bool IsUserFunc() const {
    return flag & FUNCUSER;
  }

  void SetUserFunc() {
    flag |= FUNCUSER;
  }

  bool IsInfoPrinted() const {
    return flag & FUNCINFOPRINTED;
  }

  void SetInfoPrinted() {
    flag |= FUNCINFOPRINTED;
  }

  void ResetInfoPrinted() {
    flag &= ~FUNCINFOPRINTED;
  }

  void SetNoReturn() {
    flag |= FUNCNEVERRETURN;
  }

  bool NeverReturns() const {
    return flag & FUNCNEVERRETURN;
  }

  void SetReturnStruct(MIRType &retType) {
    switch (retType.GetKind()) {
      case kTypeUnion:
      case kTypeStruct:
      case kTypeStructIncomplete:
      case kTypeClass:
      case kTypeClassIncomplete:
      case kTypeInterface:
      case kTypeInterfaceIncomplete:
        flag |= FUNCRETSTRUCT;
        break;
      default:;
    }
  }

  bool IsEmpty() const;
  bool IsClinit() const;
  uint32 GetInfo(GStrIdx strIdx) const;
  uint32 GetInfo(const std::string &string) const;
  bool IsAFormal(const MIRSymbol *st) const {
    for (MapleVector<MIRSymbol*>::const_iterator it = formals.begin(); it != formals.end(); it++) {
      if (st == *it) {
        return true;
      }
    }
    return false;
  }

  uint32 GetFormalIndex(const MIRSymbol *st) const {
    for (size_t i = 0; i < formals.size(); i++)
      if (formals[i] == st) {
        return i;
      }
    return 0xffffffff;
  }

  // tell whether this function is a Java method
  bool IsJava() const {
    return classTyIdx.GetIdx();
  }

  MIRType *GetNodeType(BaseNode &node);
  void SetUpGDBEnv();
  void ResetGDBEnv();
  MemPool *GetCodeMempool() {
    return codeMemPool;
  }

  MapleAllocator *GetCodeMemPoolAllocator() {
    return &codeMemPoolAllocator;
  }

  MapleAllocator *GetCodeMempoolAllocator() {
    return &codeMemPoolAllocator;
  }

  void NewBody();

  MIRModule *GetModule() {
    return module;
  }

  PUIdx GetPuidx() const {
    return puIdx;
  }

  void SetPuidx(PUIdx n) {
    puIdx = n;
  }

  PUIdx GetPuidxOrigin() const {
    return puIdxOrigin;
  }

  void SetPuidxOrigin(PUIdx n) {
    puIdxOrigin = n;
  }

  StIdx GetStIdx() const {
    return symbolTableIdx;
  }
  void SetStIdx(StIdx stIdx) {
    symbolTableIdx = stIdx;
  }


  MIRFuncType *GetMIRFuncType() {
    return funcType;
  }

  void SetMIRFuncType(MIRFuncType *type) {
    funcType = type;
  }


  const MapleVector<TyIdx> &GetArgumentsTyIdx() const {
    return argumentsTyIdx;
  }
  void ClearArgumentsTyIdx() {
    argumentsTyIdx.clear();
  }

  TyIdx GetArgumentsTyIdxItem(size_t n) const {
    ASSERT(n < argumentsTyIdx.size(), "array index out of range");
    return argumentsTyIdx.at(n);
  }

  size_t GetArgumentsTyIdxSize() const {
    return argumentsTyIdx.size();
  }

  const MapleVector<TypeAttrs> &GetArgumentsAttrs() const {
    return argumentsAttrs;
  }
  void ClearArgumentsAttrs() {
    argumentsAttrs.clear();
  }

  const MapleMap<GStrIdx, TyIdx> &GetGStrIdxToTyIdxMap() const {
    return typeNameTab->GetGStrIdxToTyIdxMap();
  }
  TyIdx GetTyIdxFromGStrIdx(GStrIdx idx) const {
    return typeNameTab->GetTyIdxFromGStrIdx(idx);
  }
  void SetGStrIdxToTyIdx(GStrIdx gStrIdx, TyIdx tyIdx) {
    typeNameTab->SetGStrIdxToTyIdx(gStrIdx, tyIdx);
  }

  const std::string &GetLabelTabItem(LabelIdx labidx) const {
    return labelTab->GetName(labidx);
  }

  MIRPregTable *GetPregTab() {
    return pregTab;
  }

  MIRPreg *GetPregItem(PregIdx idx) const {
    return pregTab->PregFromPregIdx(idx);
  }

  void SetPregTab(MIRPregTable *tab) {
    pregTab = tab;
  }

  MemPool *GetMemPool() {
    return dataMemPool;
  }

  BlockNode *GetBody() {
    return body;
  }

  const BlockNode *GetBody() const {
    return body;
  }

  void SetBody(BlockNode *node) {
    body = node;
  }

  SrcPosition &GetSrcPosition() {
    return srcPosition;
  }
  void SetSrcPosition(const SrcPosition &sp) {
    srcPosition = sp;
  }

  const FuncAttrs &GetFuncAttrs() const {
    return funcAttrs;
  }
  void SetFuncAttrs(const FuncAttrs &fa) {
    funcAttrs = fa;
  }
  void SetFuncAttrs(uint64 attrFlag) {
    funcAttrs.SetAttrFlag(attrFlag);
  }

  uint32 GetFlag() {
    return flag;
  }

  void SetFlag(uint32 f) {
    flag = f;
  }

  uint16 GetHashCode() {
    return hashCode;
  }

  void SetHashCode(uint16 h) {
    hashCode = h;
  }

  void SetFileIndex(uint32 fi) {
    fileIndex = fi;
  }

  const MIRInfoVector &GetInfoVector() const {
    return info;
  }
  void PushbackMIRInfo(const MIRInfoPair &pair) {
    info.push_back(pair);
  }
  void SetMIRInfoNum(size_t idx, uint32 num) {
    info[idx].second = num;
  }

  const MapleVector<bool> &InfoIsString() const {
    return infoIsString;
  }
  void PushbackIsString(bool isString) {
    infoIsString.push_back(isString);
  }

  const MapleMap<GStrIdx, MIRAliasVars> &GetAliasVarMap() const {
    return aliasVarMap;
  }
  void SetAliasVarMap(GStrIdx g, MIRAliasVars &a) {
    aliasVarMap[g] = a;
  }


  bool WithLocInfo() {
    return withLocInfo;
  }

  void SetWithLocInfo(bool s) {
    withLocInfo = s;
  }


  uint8 GetLayoutType() {
    return layoutType;
  }

  const uint8 GetLayoutType() const {
    return layoutType;
  }

  void SetLayoutType(uint8 lt) {
    layoutType = lt;
  }

  uint16 GetFrameSize() const {
    return frameSize;
  }

  void SetFrameSize(uint16 fs) {
    frameSize = fs;
  }

  uint16 GetUpFormalSize() const {
    return upFormalSize;
  }

  void SetUpFormalSize(uint16 uf) {
    upFormalSize = uf;
  }

  uint16 GetModuleId() const {
    return moduleID;
  }

  void SetModuleID(uint16 mi) {
    moduleID = mi;
  }

  uint32 GetFuncSize() const {
    return funcSize;
  }

  void SetFuncSize(uint32 fs) {
    funcSize = fs;
  }

  uint32 GetTempCount() const {
    return tempCount;
  }

  void IncTempCount() {
    tempCount++;
  }

  const uint8 *GetFormalWordsTypeTagged() const {
    return formalWordsTypeTagged;
  }

  uint8 **GetFwtAddress() {
    return &formalWordsTypeTagged;
  }

  void SetFormalWordsTypeTagged(uint8 *fwt) {
    formalWordsTypeTagged = fwt;
  }

  uint8 **GetLwtAddress() {
    return &localWordsTypeTagged;
  }

  const uint8 *GetLocalWordsTypeTagged() const {
    return localWordsTypeTagged;
  }

  void SetLocalWordsTypeTagged(uint8 *lwt) {
    localWordsTypeTagged = lwt;
  }

  uint8 **GetFwrAddress() {
    return &formalWordsRefCounted;
  }

  const uint8 *GetFormalWordsRefCounted() const {
    return formalWordsRefCounted;
  }

  void SetFormalWordsRefCounted(uint8 *fwr) {
    formalWordsRefCounted = fwr;
  }

  const uint8 *GetLocalWordsRefCounted() const {
    return localWordsRefCounted;
  }
  void SetLocalWordsRefCounted(uint8 *lwr) {
    localWordsRefCounted = lwr;
  }

  MeFunction *GetMeFunc() {
    return mefunc;
  }
  void SetMeFunc(MeFunction *func) {
    mefunc = func;
  }

  EAConnectionGraph *GetEACG() {
    return eacg;
  }
  void SetEACG(EAConnectionGraph *eacgVal) {
    eacg = eacgVal;
  }

  void SetFormals(MapleVector<MIRSymbol*> currFormals) {
    formals = currFormals;
  }

  MIRSymbol *GetFormal(size_t i) const {
    CHECK_FATAL(i < formals.size(), "array index out of range");
    return formals[i];
  }

  void SetFormal(size_t index, MIRSymbol *value) {
    CHECK_FATAL(index < formals.size(), "array index out of range");
    formals[index] = value;
  }

  size_t GetFormalCount() const {
    return formals.size();
  }

  void AddFormal(MIRSymbol *formal) {
    formals.push_back(formal);
  }

  void ClearFormals() {
    formals.clear();
  }

  uint32 GetSymbolTabSize() const {
    return symTab->GetSymbolTableSize();
  }
  MIRSymbol *GetSymbolTabItem(uint32 idx, bool checkfirst = false) const {
    return symTab->GetSymbolFromStIdx(idx, checkfirst);
  }
  const MIRSymbolTable *GetSymTab() const {
    return symTab;
  }
  MIRSymbolTable *GetSymTab() {
    return symTab;
  }

  const MIRLabelTable *GetLabelTab() const {
    return labelTab;
  }

  MIRLabelTable *GetLabelTab() {
    return labelTab;
  }

  void SetLabelTab(MIRLabelTable *currLabelTab) {
    labelTab = currLabelTab;
  }

  const MapleSet<MIRSymbol*> &GetRetRefSym() const {
    return retRefSym;
  }
  void InsertMIRSymbol(MIRSymbol *sym) {
    retRefSym.insert(sym);
  }

  MemPool *GetDataMemPool() {
    return dataMemPool;
  }

  MemPool *GetCodeMemPool() {
    return codeMemPool;
  }

  void SetCodeMemPool(MemPool *currCodemp) {
    codeMemPool = currCodemp;
  }

  MapleAllocator &GetCodeMPAllocator() {
    return codeMemPoolAllocator;
  }

 private:
  MIRModule *module;     // the module that owns this function
  PUIdx puIdx = 0;           // the PU index of this function
  PUIdx puIdxOrigin;     // the original puIdx when initial generation
  StIdx symbolTableIdx;  // the symbol table index of this function
  MIRFuncType *funcType = nullptr;
  TyIdx returnTyIdx;                // the declared return type of this function
  TyIdx classTyIdx{0};                 // class/interface type this function belongs to
  MapleVector<MIRSymbol*> formals;  // formal parameter symbols of this function
  MapleSet<MIRSymbol*> retRefSym;
  MapleVector<TyIdx> argumentsTyIdx;  // arguments types of this function
  MapleVector<TypeAttrs> argumentsAttrs;

  MIRSymbolTable *symTab = nullptr;
  MIRTypeNameTable *typeNameTab = nullptr;
  MIRLabelTable *labelTab = nullptr;
  MIRPregTable *pregTab = nullptr;
  MemPool *dataMemPool;
  MapleAllocator dataMPAllocator;
  MemPool *codeMemPool;
  MapleAllocator codeMemPoolAllocator;
  BlockNode *body = nullptr;
  SrcPosition srcPosition;
  FuncAttrs funcAttrs;
  uint32 flag = 0;
  uint16 hashCode;   // for methodmetadata order
  uint32 fileIndex = 0;  // this function belongs to which file, used by VM for plugin manager
  MIRInfoVector info;
  MapleVector<bool> infoIsString;               // tells if an entry has string value
  MapleMap<GStrIdx, MIRAliasVars> aliasVarMap;  // source code alias variables for debuginfo
  bool withLocInfo;

  uint8_t layoutType;
  uint16 frameSize;
  uint16 upFormalSize;
  uint16 moduleID;
  uint32 funcSize;                         // size of code in words
  uint32 tempCount;
  uint8 *formalWordsTypeTagged = nullptr;  // bit vector where the Nth bit tells whether
  // the Nth word in the formal parameters area
  // addressed upward from %%FP (that means
  // the word at location (%%FP + N*4)) has
  // typetag; if yes, the typetag is the word
  // at (%%FP + N*4 + 4); the bitvector's size
  // is given by BlockSize2BitvectorSize(upFormalSize)
  uint8 *localWordsTypeTagged = nullptr;  // bit vector where the Nth bit tells whether
  // the Nth word in the local stack frame
  // addressed downward from %%FP (that means
  // the word at location (%%FP - N*4)) has
  // typetag; if yes, the typetag is the word
  // at (%%FP - N*4 + 4); the bitvector's size
  // is given by BlockSize2BitvectorSize(framesize)
  uint8 *formalWordsRefCounted = nullptr;  // bit vector where the Nth bit tells whether
  // the Nth word in the formal parameters area
  // addressed upward from %%FP (that means
  // the word at location (%%FP + N*4)) points to
  // a dynamic memory block that needs reference
  // count; the bitvector's size is given by
  // BlockSize2BitvectorSize(upFormalSize)
  uint8 *localWordsRefCounted = nullptr;  // bit vector where the Nth bit tells whether
  // the Nth word in the local stack frame
  // addressed downward from %%FP (that means
  // the word at location (%%FP - N*4)) points to
  // a dynamic memory block that needs reference
  // count; the bitvector's size is given by
  // BlockSize2BitvectorSize(framesize)
  // uint16 numlabels; // removed. label table size
  // StmtNode **lbl2stmt; // lbl2stmt table, removed;
  // to hold unmangled class and function names
  MeFunction *mefunc;
  EAConnectionGraph *eacg;
  GStrIdx baseClassStrIdx;  // the string table index of base class name
  GStrIdx baseFuncStrIdx;   // the string table index of base function name
  // the string table index of base function name mangled with type info
  GStrIdx baseFuncWithTypeStrIdx;
  // funcname + types of args, no type of retv
  GStrIdx signatureStrIdx;

  void DumpFlavorLoweredThanMmpl() const;
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_FUNCTION_H
