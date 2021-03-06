; REQUIRES: x86
; RUN: llvm-as %s -o %t.o
; RUN: llvm-as %p/Inputs/internalize-exportdyn.ll -o %t2.o
; RUN: ld.lld -m elf_x86_64 %t.o %t2.o -o %t2 --export-dynamic -save-temps
; RUN: llvm-dis < %t2.lto.bc | FileCheck %s

target triple = "x86_64-unknown-linux-gnu"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @_start() {
  ret void
}

define void @foo() {
  ret void
}

define hidden void @bar() {
  ret void
}

define linkonce_odr void @zed() local_unnamed_addr {
  ret void
}

define linkonce_odr void @zed2() unnamed_addr {
  ret void
}

define linkonce_odr void @bah() {
  ret void
}

define linkonce_odr void @baz() {
  ret void
}

@use_baz = global void ()* @baz

; Check what gets internalized.
; CHECK: define void @_start()
; CHECK: define void @foo()
; CHECK: define internal void @bar()
; CHECK: define internal void @zed()
; CHECK: define internal void @zed2()
; CHECK: define weak_odr void @bah()
; CHECK: define weak_odr void @baz()
