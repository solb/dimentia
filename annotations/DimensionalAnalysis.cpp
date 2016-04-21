#include "DimensionalAnalysis.h"

#include <llvm/Support/raw_ostream.h>

#include "TraceVariablesNg.h"

using namespace llvm;
using std::move;
using std::string;

dimens_var::dimens_var(unsigned long hash, string &&str) :
    hash(hash),
    str(move(str)) {}

dimens_var::~dimens_var() = default;

bool dimens_var::operator==(const dimens_var &other) const {
  return this->hash == other.hash;
}

dimens_var::operator unsigned long() const {
  return hash;
}

dimens_var::operator const string &() const {
  return str;
}

struct DIVariable_var : dimens_var {
  DIVariable_var(DIVariable *ptr) :
      dimens_var((unsigned long) ptr,
          ptr ? TraceVariablesNg::str(*ptr) : "") {}
};

char DimensionalAnalysis::ID = 0;

DimensionalAnalysis::DimensionalAnalysis() :
    ModulePass(ID),
    variables(),
    indices(),
    groupings(nullptr) {}

void DimensionalAnalysis::getAnalysisUsage(llvm::AnalysisUsage &info) const {
  info.addRequired<TraceVariablesNg>();
}

bool DimensionalAnalysis::runOnModule(llvm::Module &module) {
  // We cannot allow modification of this structure or the (parallel) indices won't be stable!
  const TraceVariablesNg &groupings = getAnalysis<TraceVariablesNg>();
  this->groupings = &groupings;

  variables.reserve(groupings.vals.size());
  for(auto mapping : groupings.vals)
    insert(DIVariable_var(mapping.first));

  return false;
}

void DimensionalAnalysis::print(llvm::raw_ostream &stream, const llvm::Module *module) const {
  for(const dimens_var &each : variables)
    stream << (const string &) each << '\n';
}

void DimensionalAnalysis::insert(dimens_var &&var) {
  assert(var);

  variables.push_back(var);
  indices.emplace(move(var), indices.size());
}

static RegisterPass<DimensionalAnalysis> dimens("dimens", "Dimensional Analysis", true, true);
