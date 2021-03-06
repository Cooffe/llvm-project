//===-- OpenMP.cpp -- Open MP directive lowering --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Coding style: https://mlir.llvm.org/getting_started/DeveloperGuide/
//
//===----------------------------------------------------------------------===//

#include "flang/Lower/OpenMP.h"
#include "flang/Lower/Bridge.h"
#include "flang/Lower/FIRBuilder.h"
#include "flang/Lower/PFTBuilder.h"
#include "flang/Parser/parse-tree.h"
#include "flang/Semantics/tools.h"
#include "mlir/Dialect/OpenMP/OpenMPDialect.h"
#include "llvm/Frontend/OpenMP/OMPConstants.h"

#define TODO() llvm_unreachable("not yet implemented")

static void genOMP(Fortran::lower::AbstractConverter &converter,
                   Fortran::lower::pft::Evaluation &eval,
                   const Fortran::parser::OpenMPSimpleStandaloneConstruct
                       &simpleStandaloneConstruct) {
  const auto &directive =
      std::get<Fortran::parser::OmpSimpleStandaloneDirective>(
          simpleStandaloneConstruct.t);
  switch (directive.v) {
  default:
    break;
  case llvm::omp::Directive::OMPD_barrier:
    converter.getFirOpBuilder().create<mlir::omp::BarrierOp>(
        converter.getCurrentLocation());
    break;
  case llvm::omp::Directive::OMPD_taskwait:
    converter.getFirOpBuilder().create<mlir::omp::TaskwaitOp>(
        converter.getCurrentLocation());
    break;
  case llvm::omp::Directive::OMPD_taskyield:
    converter.getFirOpBuilder().create<mlir::omp::TaskyieldOp>(
        converter.getCurrentLocation());
    break;
  case llvm::omp::Directive::OMPD_target_enter_data:
    TODO();
  case llvm::omp::Directive::OMPD_target_exit_data:
    TODO();
  case llvm::omp::Directive::OMPD_target_update:
    TODO();
  case llvm::omp::Directive::OMPD_ordered:
    TODO();
  }
}

static void
genOMP(Fortran::lower::AbstractConverter &converter,
       Fortran::lower::pft::Evaluation &eval,
       const Fortran::parser::OpenMPStandaloneConstruct &standaloneConstruct) {
  std::visit(
      Fortran::common::visitors{
          [&](const Fortran::parser::OpenMPSimpleStandaloneConstruct
                  &simpleStandaloneConstruct) {
            genOMP(converter, eval, simpleStandaloneConstruct);
          },
          [&](const Fortran::parser::OpenMPFlushConstruct &flushConstruct) {
            TODO();
          },
          [&](const Fortran::parser::OpenMPCancelConstruct &cancelConstruct) {
            TODO();
          },
          [&](const Fortran::parser::OpenMPCancellationPointConstruct
                  &cancellationPointConstruct) { TODO(); },
      },
      standaloneConstruct.u);
}

static void
genOMP(Fortran::lower::AbstractConverter &converter,
       Fortran::lower::pft::Evaluation &eval,
       const Fortran::parser::OpenMPBlockConstruct &blockConstruct) {
  const auto &blockDirective =
      std::get<Fortran::parser::OmpBeginBlockDirective>(blockConstruct.t);
  const auto &parallelDirective =
      std::get<Fortran::parser::OmpBlockDirective>(blockDirective.t);
  if (parallelDirective.v == llvm::omp::OMPD_parallel) {
    auto &firOpBuilder = converter.getFirOpBuilder();
    auto currentLocation = converter.getCurrentLocation();
    auto insertPt = firOpBuilder.saveInsertionPoint();

    // Clauses.
    // FIXME: Add support for other clauses.
    mlir::Value numThreads;

    const auto &parallelOpClauseList =
        std::get<Fortran::parser::OmpClauseList>(blockDirective.t);
    for (const auto &clause : parallelOpClauseList.v) {
      if (const auto &numThreadsClause =
              std::get_if<Fortran::parser::OmpClause::NumThreads>(&clause.u)) {
        // OMPIRBuilder expects `NUM_THREAD` clause as a `Value`.
        numThreads = converter.genExprValue(
            *Fortran::semantics::GetExpr(numThreadsClause->v));
      }
    }
    llvm::ArrayRef<mlir::Type> argTy;
    Attribute defaultValue, procBindValue;
    // Create and insert the operation.
    // Create the Op with empty ranges for clauses that are yet to be lowered.
    auto parallelOp = firOpBuilder.create<mlir::omp::ParallelOp>(
        currentLocation, argTy, Value(), numThreads,
        defaultValue.dyn_cast_or_null<StringAttr>(), ValueRange(), ValueRange(),
        ValueRange(), ValueRange(),
        procBindValue.dyn_cast_or_null<StringAttr>());
    firOpBuilder.createBlock(&parallelOp.getRegion());
    auto &block = parallelOp.getRegion().back();
    firOpBuilder.setInsertionPointToStart(&block);
    // Ensure the block is well-formed.
    firOpBuilder.create<mlir::omp::TerminatorOp>(currentLocation);
    firOpBuilder.restoreInsertionPoint(insertPt);
  }
}

void Fortran::lower::genOpenMPConstruct(
    Fortran::lower::AbstractConverter &converter,
    Fortran::lower::pft::Evaluation &eval,
    const Fortran::parser::OpenMPConstruct &ompConstruct) {

  std::visit(
      common::visitors{
          [&](const Fortran::parser::OpenMPStandaloneConstruct
                  &standaloneConstruct) {
            genOMP(converter, eval, standaloneConstruct);
          },
          [&](const Fortran::parser::OpenMPSectionsConstruct
                  &sectionsConstruct) { TODO(); },
          [&](const Fortran::parser::OpenMPLoopConstruct &loopConstruct) {
            TODO();
          },
          [&](const Fortran::parser::OpenMPBlockConstruct &blockConstruct) {
            genOMP(converter, eval, blockConstruct);
          },
          [&](const Fortran::parser::OpenMPAtomicConstruct &atomicConstruct) {
            TODO();
          },
          [&](const Fortran::parser::OpenMPCriticalConstruct
                  &criticalConstruct) { TODO(); },
      },
      ompConstruct.u);
}

void Fortran::lower::genOpenMPEndLoop(
    Fortran::lower::AbstractConverter &, Fortran::lower::pft::Evaluation &,
    const Fortran::parser::OmpEndLoopDirective &) {
  TODO();
}
