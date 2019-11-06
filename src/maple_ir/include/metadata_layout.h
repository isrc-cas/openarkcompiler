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
#ifndef MAPLE_IR_INCLUDE_METADATA_LAYOUT_H
#define MAPLE_IR_INCLUDE_METADATA_LAYOUT_H
#include <cstdlib>

// metadata layout is shared between maple compiler and runtime, thus not in namespace maplert

// data types for address offset
#ifdef USE_32BIT_REF
using AddrOffset = int32_t;
#else
using AddrOffset = intptr_t;
#endif

// some of the reference field of metadata is stored as relative offset
// for example, declaring class of Fields/Methods
// which can be negative
#ifdef USE_32BIT_REF
using MetaRef = uint32_t;      // consistent with reffield_t in address.h
#else
using MetaRef = uintptr_t;     // consistent iwth reffield_t in address.h
#endif // USE_32BIT_REF

static inline void *DecodeValueAsRelOffset32(const int32_t *addr) {
  intptr_t realAddr = reinterpret_cast<intptr_t>(addr) + *addr;
  return reinterpret_cast<void*>(realAddr);
}

static inline void *DecodeValueAsRelOffset64(const int64_t *addr) {
  intptr_t realAddr = reinterpret_cast<intptr_t>(addr) + *addr;
  return reinterpret_cast<void*>(realAddr);
}

static inline void EncodeValueAsRelOffset32(const void *value, int32_t *addr) {
  int32_t offset = reinterpret_cast<intptr_t>(value) - reinterpret_cast<intptr_t>(addr);
  *addr = offset;
  if (DecodeValueAsRelOffset32(addr) != value) {
    std::abort();
  }
}

static inline void EncodeValueAsRelOffset64(const void *value, int64_t *addr) {
  int64_t offset = reinterpret_cast<intptr_t>(value) - reinterpret_cast<intptr_t>(addr);
  *addr = offset;
  if (DecodeValueAsRelOffset64(addr) != value) {
    std::abort();
  }
}

struct DataRefOffset32 {
  int32_t refOffset;
  inline void SetDataRef(void *ref);
  inline void *GetDataRef() const;
};

// DataRefOffset is meant to have pointer size.
struct DataRefOffset {
  intptr_t refOffset;
  inline void SetDataRef(void *ref);
  inline void *GetDataRef() const;
};

/* DataRef aims for reference to data generated by maple compiler.
   DataRef field allows 4 formats of value:
   0. "label_name" for direct reference
   1. "label_name - . + 2" for historical compact metadata reference
   2. "label_name - . + 1" for reference in offset format
   3. "indirect.label_name - . + 3" for indirect reference
      this format aims to support lld which does not support expression "global_symbol - ."
   DataRef is self-decoded by also encoding the format and is declared for binary compatibility.
   If no compatibility problem is involved, is preferred.
 */

enum DataRefFormat {
  kDataRefIsDirect   = 0,
  kDataRefIsCompact  = 1, // read-only
  kDataRefIsOffset   = 2,
  kDataRefIsIndirect = 3, // read-only
  kDataRefBitMask    = 3,
};

struct DataRef32 {
  // be careful when *refVal* is treated as an offset which is a signed integer actually.
  uint32_t refVal;
  inline void *GetDataRef() const;
  inline void SetDataRef(void *ref, DataRefFormat format = kDataRefIsDirect);
};

// DataRef is meant to have pointer size.
struct DataRef {
  void *refVal;
  inline void *GetDataRef() const;
  inline void SetDataRef(void *ref, DataRefFormat format = kDataRefIsDirect);
};

// MethodMeta defined in MethodMeta.h
// FieldMeta  defined in FieldMeta.h

// MethodDesc contains MethodMetadata and stack map
struct MethodDesc {
  // relative offset for method metadata relative to current PC.
  // method metadata is in compact format if this offset is odd.
  uint32_t metadataOffset;

  uint16_t localRefOffset;
  uint16_t localRefNumber;

  // stack map for a methed might be placed here
};

// Note: class init in maplebe and cg is highly dependent on this type.
// update aarch64rtsupport.h if you modify this definition.
struct ClassMetadataRO {
  char *className;
  int64_t fields;  // point to info of fields
  int64_t methods; // point to info of methods
  union {  // Element classinfo of array, others parent classinfo
    DataRef superclass;
    struct ClassMetadata *componentClass;
  };

  uint16_t numOfFields;
  uint16_t numOfMethods;

#ifndef USE_32BIT_REF
  uint16_t flag;
  uint16_t numOfSuperclasses;
  uint32_t padding;
#endif // !USE_32BIT_REF

  uint32_t mod; // 32 bit 0X00000000    modifier: 0x00FFFFFF   override: 0x80000000
  int32_t annotation;
  int32_t clinitAddr;
};

static constexpr size_t PageSize = 4096;

// Note there is no state to indicate a class is already initialized.
// Any state beyond listed below is treated as initialized.
enum ClassInitState {
  kClassInitStateMin   = 0,
  kClassUninitialized  = 1,
  kClassInitializing   = 2,
  kClassInitFailed     = 3,
  kClassInitialized    = 4,
  kClassInitStateMax   = 4,
};

enum SEGVAddr {
  kSEGVAddrRangeStart            = PageSize + 0,

  // Note any readable address is treated as Initialized.
  kSEGVAddrForClassInitStateMin  = kSEGVAddrRangeStart + kClassInitStateMin,
  kSEGVAddrForClassUninitialized = kSEGVAddrForClassInitStateMin + kClassUninitialized,
  kSEGVAddrForClassInitializing  = kSEGVAddrForClassInitStateMin + kClassInitializing,
  kSEGVAddrForClassInitFailed    = kSEGVAddrForClassInitStateMin + kClassInitFailed,
  kSEGVAddrFoClassInitStateMax   = kSEGVAddrForClassInitStateMin + kClassInitStateMax,

  kSEGVAddrRangeEnd,
};

struct ClassMetadata {
  // object common fields
  MetaRef shadow;  // point to classinfo of java/lang/Class
  int32_t monitor;

  // other fields
  uint16_t clIndex; // 8bit ClassLoader index, used for querying the address of related ClassLoader instance.
  union {
    uint16_t objSize;
    uint16_t componentSize;
  } sizeInfo;

#ifdef USE_32BIT_REF // for alignment purpose
  uint16_t flag;
  uint16_t numOfSuperclasses;
#endif // USE_32BIT_REF

  void *iTable;  // iTable of current class, used for virtual call, will insert the content into classinfo
  void *vTable;  // vTable of current class, used for interface call, will insert the content into classinfo
  void *gctib; // for rc

  union {
    DataRef classinforo64; // ifndef USE_32BIT_REF
    struct {
      DataRef32 classinforo32; // ifdef USE_32BIT_REF
      uint32_t cacheFalseClass;
    };
  };

  union {
    void *initState; // a readable address for initState means initialized
    void *cacheTrueClass;
  };

 public:
  static inline intptr_t OffsetOfInitState() {
    ClassMetadata *base = nullptr;
    return reinterpret_cast<intptr_t>(&(base->initState));
  }

  bool IsInitialized();

  ClassInitState GetInitState();
  void *GetInitStateRawValue() {
    return __atomic_load_n(&initState, __ATOMIC_ACQUIRE);
  }

  void SetInitState(ClassInitState state);
  void SetInitStateRawValue(void *val) {
    __atomic_store_n(&initState, val, __ATOMIC_RELEASE);
  }
};

// function to set Class/Field/Method metadata's shadow field to avoid type conversion
// Note 1: here we don't do NULL-check and type-compatibility check
// NOte 2: C should be of jclass/ClassMetata* type
template<typename M, typename C>
static inline void MRTSetMetadataShadow(M *meta, C cls) {
  meta->shadow = (MetaRef)(uintptr_t)cls;
}

#endif // METADATA_LAYOUT_H
