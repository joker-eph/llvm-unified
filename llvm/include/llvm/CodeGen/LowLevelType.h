//== llvm/CodeGen/GlobalISel/LowLevelType.h -------------------- -*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// Implement a low-level type suitable for MachineInstr level instruction
/// selection.
///
/// For a type attached to a MachineInstr, we only care about 2 details: total
/// size and the number of vector lanes (if any). Accordingly, there are 3
/// possible valid type-kinds:
///
///    * `unsized` for labels etc
///    * `sN` for scalars and aggregates
///    * `<N x sM>` for vectors, which must have at least 2 elements.
///
/// Other information required for correct selection is expected to be carried
/// by the opcode, or non-type flags. For example the distinction between G_ADD
/// and G_FADD for int/float or fast-math flags.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_GLOBALISEL_LOWLEVELTYPE_H
#define LLVM_CODEGEN_GLOBALISEL_LOWLEVELTYPE_H

#include <cassert>
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/CodeGen/ValueTypes.h"

namespace llvm {

class LLVMContext;
class Type;
class raw_ostream;

class LLT {
public:
  enum TypeKind : uint16_t {
    Invalid,
    Scalar,
    Vector,
    Unsized,
  };

  /// \brief get a low-level scalar or aggregate "bag of bits".
  static LLT scalar(unsigned SizeInBits) {
    return LLT{Scalar, 1, SizeInBits};
  }

  /// \brief get a low-level vector of some number of elements and element
  /// width. \p NumElements must be at least 2.
  static LLT vector(uint16_t NumElements, unsigned ScalarSizeInBits) {
    assert(NumElements > 1 && "invalid number of vector elements");
    return LLT{Vector, NumElements, ScalarSizeInBits};
  }

  /// \brief get a low-level vector of some number of elements and element
  /// type
  static LLT vector(uint16_t NumElements, LLT ScalarTy) {
    assert(NumElements > 1 && "invalid number of vector elements");
    assert(ScalarTy.isScalar() && "invalid vector element type");
    return LLT{Vector, NumElements, ScalarTy.getSizeInBits()};
  }

  /// \brief get an unsized but valid low-level type (e.g. for a label).

  static LLT unsized() {
    return LLT{Unsized, 1, 0};
  }

  explicit LLT(TypeKind Kind, uint16_t NumElements, unsigned ScalarSizeInBits)
    : ScalarSize(ScalarSizeInBits), NumElements(NumElements), Kind(Kind) {
    assert((Kind != Vector || NumElements > 1) &&
           "invalid number of vector elements");
  }

  explicit LLT() : ScalarSize(0), NumElements(0), Kind(Invalid) {}

  /// \brief construct a low-level type based on an LLVM type.
  explicit LLT(const Type &Ty);

  bool isValid() const { return Kind != Invalid; }

  bool isScalar() const { return Kind == Scalar; }

  bool isVector() const { return Kind == Vector; }

  bool isSized() const { return Kind == Scalar || Kind == Vector; }

  /// \brief Returns the number of elements in a vector LLT. Must only be called
  /// on vector types.
  uint16_t getNumElements() const {
    assert(isVector() && "cannot get number of elements on scalar/aggregate");
    return NumElements;
  }

  /// \brief Returns the total size of the type. Must only be called on sized
  /// types.
  unsigned getSizeInBits() const {
    assert(isSized() && "attempt to get size of unsized type");
    return ScalarSize * NumElements;
  }

  unsigned getScalarSizeInBits() const {
    assert(isSized() && "cannot get size of this type");
    return ScalarSize;
  }

  /// \brief Returns the vector's element type. Only valid for vector types.
  LLT getElementType() const {
    assert(isVector() && "cannot get element type of scalar/aggregate");
    return scalar(ScalarSize);
  }

  /// \brief get a low-level type with half the size of the original, by halving
  /// the size of the scalar type involved. For example `s32` will become
  /// `s16`, `<2 x s32>` will become `<2 x s16>`.
  LLT halfScalarSize() const {
    assert(isSized() && "cannot change size of this type");
    return LLT{Kind, NumElements, ScalarSize / 2};
  }

  /// \brief get a low-level type with twice the size of the original, by
  /// doubling the size of the scalar type involved. For example `s32` will
  /// become `s64`, `<2 x s32>` will become `<2 x s64>`.
  LLT doubleScalarSize() const {
    assert(isSized() && "cannot change size of this type");
    return LLT{Kind, NumElements, ScalarSize * 2};
  }

  /// \brief get a low-level type with half the size of the original, by halving
  /// the number of vector elements of the scalar type involved. The source must
  /// be a vector type with an even number of elements. For example `<4 x
  /// s32>` will become `<2 x s32>`, `<2 x s32>` will become `s32`.
  LLT halfElements() const {
    assert(isVector() && NumElements % 2 == 0 && "cannot half odd vector");
    if (NumElements == 2)
      return scalar(ScalarSize);

    return LLT{Vector, static_cast<uint16_t>(NumElements / 2), ScalarSize};
  }

  /// \brief get a low-level type with twice the size of the original, by
  /// doubling the number of vector elements of the scalar type involved. The
  /// source must be a vector type. For example `<2 x s32>` will become `<4 x
  /// s32>`. Doubling the number of elements in sN produces <2 x sN>.
  LLT doubleElements() const {
    return LLT{Vector, static_cast<uint16_t>(NumElements * 2), ScalarSize};
  }

  void print(raw_ostream &OS) const;

  bool operator ==(const LLT &RHS) const {
    return Kind == RHS.Kind && ScalarSize == RHS.ScalarSize &&
           NumElements == RHS.NumElements;
  }

  friend struct DenseMapInfo<LLT>;
private:
  unsigned ScalarSize;
  uint16_t NumElements;
  TypeKind Kind;
};

template<> struct DenseMapInfo<LLT> {
  static inline LLT getEmptyKey() {
    return LLT{LLT::Invalid, 0, -1u};
  }
  static inline LLT getTombstoneKey() {
    return LLT{LLT::Invalid, 0, -2u};
  }
  static inline unsigned getHashValue(const LLT &Ty) {
    uint64_t Val = ((uint64_t)Ty.ScalarSize << 32) |
                   ((uint64_t)Ty.NumElements << 16) | (uint64_t)Ty.Kind;
    return DenseMapInfo<uint64_t>::getHashValue(Val);
  }
  static bool isEqual(const LLT &LHS, const LLT &RHS) {
    return LHS == RHS;
  }
};

}

#endif
