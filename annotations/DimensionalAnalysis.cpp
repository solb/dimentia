#include "DimensionalAnalysis.h"

#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/raw_ostream.h>

#include "TraceVariablesNg.h"

using namespace llvm;
using std::move;
using std::string;
using std::vector;

static string val_str(Value &obj) {
  string res;
  raw_string_ostream stm(res);
  obj.printAsOperand(stm, false);
  stm.flush();
  return res;
}

dimens_var::dimens_var(void *hash, string &&str, bool constant) :
    hash((unsigned long) hash),
    str(move(str)),
    constant(constant) {}

dimens_var::dimens_var(DIVariable &var) :
    dimens_var(&var,
        TraceVariablesNg::str(var)) {}

dimens_var::dimens_var(Value &val) :
    dimens_var(&val,
        val_str(val),
        isa<Constant>(&val)) {}

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

bool dimens_var::isa_constant() const {
  return constant;
}

char DimensionalAnalysis::ID = 0;

DimensionalAnalysis::DimensionalAnalysis() :
    ModulePass(ID),
    variables(),
    indices(),
    equations(),
    groupings(nullptr) {}

void DimensionalAnalysis::getAnalysisUsage(llvm::AnalysisUsage &info) const {
  info.addRequired<TraceVariablesNg>();
}

bool DimensionalAnalysis::runOnModule(llvm::Module &module) {
  // We cannot allow modification of this structure or the (parallel) indices won't be stable!
  const TraceVariablesNg &groupings = getAnalysis<TraceVariablesNg>();
  this->groupings = &groupings;

  // Indices less than groupings.vals.size() correspond to source variables.
  variables.reserve(groupings.vals.size());
  for(auto mapping : groupings.vals)
    insert(*mapping.first);
  // From now on, whenever we encounter a new temporary, we'll insert() it, assigning it a larger index.

  // Process the program's instructions.
  for(Function &function : module)
    for(BasicBlock &block : function.getBasicBlockList())
      for(Instruction &inst : block.getInstList())
        instruction_opdecode(inst);

  // Smooth the equations matrix's jagged boundaries.
  index_type cols = variables.size();
  for(vector<int> &row : equations) {
    assert(row.size() <= cols);
    row.resize(cols);
  }

  return false;
}

void DimensionalAnalysis::print(llvm::raw_ostream &stream, const llvm::Module *module) const {
  for(const dimens_var &each : variables)
    stream << (const string &) each << ' ';
  stream << '\n';
  for(const vector<int> &row : equations) {
    for(index_type ix = 0, sz = row.size(); ix < sz; ++ix)
      stream << format_decimal(row[ix], ((const string &) variables[ix]).size()) << ' ';
    stream << '\n';
  }
}

void DimensionalAnalysis::instruction_opdecode(Instruction &inst) {
  int multiplier = -1;
  switch(inst.getOpcode()) {
    case Instruction::Add:
    case Instruction::FAdd:
      multiplier = 1;
    case Instruction::Sub:
    case Instruction::FSub:
    case Instruction::ICmp:
    case Instruction::FCmp:
    case Instruction::PHI:
      outs() << "Processing instruction: " << inst << '\n';
      for(Use &op : inst.operands())
        instruction_setequal(inst, *op);
      break;
  }
}

void DimensionalAnalysis::instruction_setequal(const dimens_var &dest, const dimens_var &src) {
  if(src.isa_constant() || dest.isa_constant())
    return;

  outs() << "\tdeg(" << (const string &) dest << ") = deg(" << (const string &) src << ")\n";
  vector<int> equation;
  elem(equation, index(dest)) = 1;
  elem(equation, index(src)) = -1;
  equations.push_back(move(equation));
}

int &DimensionalAnalysis::elem(vector<int> &arr, DimensionalAnalysis::index_type idx) {
  if(idx >= arr.size())
    arr.resize(idx + 1);
  return arr[idx];
}

DimensionalAnalysis::index_type DimensionalAnalysis::index(const dimens_var &var) {
  return indices.count(var) ? indices[var] : insert(var);
}

DimensionalAnalysis::index_type DimensionalAnalysis::insert(const dimens_var &var) {
  assert(!indices.count(var));

  index_type ind = variables.size();
  variables.push_back(var);
  indices.emplace(var, indices.size());
  assert(variables.size() == indices.size());

  return ind;
}

static RegisterPass<DimensionalAnalysis> dimens("dimens", "Dimensional Analysis", true, true);
