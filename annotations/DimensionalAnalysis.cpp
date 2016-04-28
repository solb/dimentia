#include "DimensionalAnalysis.h"

#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/raw_ostream.h>
#include <lapacke.h>

#include "TraceVariablesNg.h"

using namespace llvm;
using std::min;
using std::move;
using std::string;
using std::vector;

#define TRACE(x) errs() << #x << " = " << x << "\n"
#define _ << " _ " <<

#define OFFSET_START_BIT         48
#define OFFSET_BIT_WIDTH(ptr_ty) (8 * sizeof(ptr_ty) - OFFSET_START_BIT)

static bool is_const(const Value *obj) {
  return obj && isa<Constant>(*obj) && !isa<GlobalValue>(*obj) && !isa<ConstantExpr>(*obj);
}

static string soff_str(const StructType &type, uint64_t offset) {
  string res;
  raw_string_ostream stm(res);
  stm << type.getName() << '[' << offset << ']';
  stm.flush();
  return res;
}

static string val_str(const Value &obj) {
  string res;
  raw_string_ostream stm(res);
  obj.printAsOperand(stm, false);
  stm.flush();
  return res;
}

const TraceVariablesNg *dimens_var::lookup = nullptr;

dimens_var::dimens_var(const void *hash, string &&str, bool constant) :
    hash((unsigned long) hash),
    str(move(str)),
    constant(constant),
    svar(nullptr) {}

dimens_var::dimens_var(const StructType &typ, const APInt &off) :
    dimens_var(&typ,
        soff_str(typ, off.getZExtValue())) {
  assert(off.getActiveBits() <= OFFSET_BIT_WIDTH(hash) && "ERROR: Struct offset too large to store!");
  // Make sure we're distinguished from the object itself, even if our offset is zero!
  hash |= 0x1;
  // Store our offset in the high-order bits, since the x86-64 address bus isn't as wide as the word size.
  hash |= off.getZExtValue() << OFFSET_START_BIT;
}

dimens_var::dimens_var(const DIVariable &var) :
    dimens_var(&var,
        TraceVariablesNg::str(var)) {}

dimens_var::dimens_var(Value &val) :
    dimens_var(&val,
        val_str(val),
        is_const(&val)) {
  assert(lookup);

  // See whether this register has a corresponding source variable.
  if(lookup->vars.count(&val))
    // Any arbitrary one of the mappings is fine, because they're all related by equations.
    svar = *lookup->vars.at(&val).begin();
}

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

dimens_var::operator DIVariable *() const {
  return svar;
}

bool dimens_var::isa_constant() const {
  return constant;
}

char DimensionalAnalysis::ID = 0;

DimensionalAnalysis::DimensionalAnalysis() :
    ModulePass(ID),
    module(nullptr),
    indirections(),
    variables(),
    indices(),
    equations(),
    dimensionless(),
    groupings(nullptr) {}

void DimensionalAnalysis::getAnalysisUsage(llvm::AnalysisUsage &info) const {
  info.addRequired<TraceVariablesNg>();
}

bool DimensionalAnalysis::runOnModule(llvm::Module &module) {
  // We cannot allow modification of this structure or the (parallel) indices won't be stable!
  const TraceVariablesNg &groupings = getAnalysis<TraceVariablesNg>();
  this->groupings = &groupings;
  dimens_var::lookup = &groupings;
  this->module = &module;

  // Indices less than groupings.vals.size() correspond to source variables.
  index_type first_temporary = groupings.vals.size();
  variables.reserve(first_temporary);
  for(auto mapping : groupings.vals)
    insert(*mapping.first);
  // From now on, whenever we encounter a new temporary, we'll insert() it, assigning it a larger index.

  for(auto revmap : groupings.vars)
    if(revmap.second.size() > 1)
      for(auto lhs = revmap.second.begin(), end = revmap.second.end(); lhs != end; ++lhs) {
        auto rhs = lhs;
        for(++rhs; rhs != end; ++rhs) {
          errs() << "Source variable analysis revealed that:";
          instruction_setequal(**lhs, **rhs);
        }
      }

  // Process the program's instructions.
  for(Function &function : module)
    for(BasicBlock &block : function.getBasicBlockList())
      for(Instruction &inst : block.getInstList())
        instruction_opdecode(inst);

  // Make sure no temporaries were erroneously added for registers already associated with a source variable.
  for(dimens_var each : variables)
    assert(!(DIVariable *) each);

  // Smooth the equations matrix's jagged boundaries.
  index_type cols = variables.size();
  for(vector<int> &row : equations) {
    assert(row.size() <= cols);
    row.resize(cols);
  }

  // Perform the actual dimensionality calculations.
  calcDimensionless();

  // Trim out temporaries to leave only source variables in our output.
  dimensionless.erase(remove_if(dimensionless.begin(), dimensionless.end(), [this, first_temporary](int index) {
    return index >= first_temporary && !(variables[index] & 0x1);
  }), dimensionless.end());
  return false;
}

void DimensionalAnalysis::print(llvm::raw_ostream &stream, const llvm::Module *module) const {
  // Here's the matrix we sent to the solver.
  for(const dimens_var &each : variables)
    stream << (const string &) each << ' ';
  stream << '\n';
  for(const vector<int> &row : equations) {
    for(index_type ix = 0, sz = row.size(); ix < sz; ++ix)
      stream << format_decimal(row[ix], ((const string &) variables[ix]).size()) << ' ';
    stream << '\n';
  }

  // And the "winners" are...
  stream << "Found " << dimensionless.size() << " dimensionless variables:\n";
  for(int index : dimensionless)
    stream << (const string &) variables[index] << '\n';
}

void DimensionalAnalysis::calcDimensionless() {
  int rows = equations.size();
  int cols = equations[0].size();

  char cN = 'N';
  char cA = 'A';

  dimensionless.clear();

  double* A = new double[rows*cols];
  double* sigmas = new double[rows+cols];
  int work_sz = (rows+cols)*30;
  double* work = new double[work_sz];
  double* Vt = new double[cols*cols];
  int ldvt = cols;
  int info;

  for (int i = 0; i < rows; ++i)
    for (int j = 0; j < cols; ++j)
      A[i+j*rows] = equations[i][j];

  dgesvd_(&cN, &cA, &rows, &cols, A, &rows,
          sigmas, NULL, &rows, Vt, &ldvt, work, &work_sz, &info);
  assert(info == 0);

  for (int i = min(cols, rows); i < cols; ++i)
    sigmas[i] = 0;

  const double eps = 1e-9;
  for (int j = 0; j < cols; ++j) {
    bool good = false;
    for (int i = 0; i < cols; ++i) {
      if (fabs(sigmas[i]) < eps && fabs(Vt[i+j*cols]) > eps) {
        good = true;
      }
    }
    if (!good) {
      dimensionless.push_back(j);
    }
  }

  delete[] A;
  delete[] sigmas;
  delete[] work;
  delete[] Vt;

  getBadEqns();
}

void DimensionalAnalysis::getBadEqns() {
  int rows = equations.size();
  int rowsM1 = rows - 1;
  int cols = equations[0].size();

  char cN = 'N';
  char cA = 'A';

  double* A = new double[rows*cols];
  double* sigmas = new double[rows+cols];
  int work_sz = (rows+cols)*30;
  double* work = new double[work_sz];
  double* Vt = new double[cols*cols];
  int ldvt = cols;
  int info;

  bad_eqns.clear();
  vector<int> new_dimensionless;

  for (int rem_row = 0; rem_row < rows; ++rem_row) {
    new_dimensionless.clear();

    for (int i = 0, ci = 0; i < rows; ++i, ++ci) {
      if (i == rem_row) {
        --ci;
        continue;
      }
      for (int j = 0; j < cols; ++j)
        A[ci+j*rowsM1] = equations[i][j];
    }

    dgesvd_(&cN, &cA, &rowsM1, &cols, A, &rowsM1,
            sigmas, NULL, &rowsM1, Vt, &ldvt, work, &work_sz, &info);
    assert(info == 0);

    for (int i = min(cols, rowsM1); i < cols; ++i)
      sigmas[i] = 0;

    const double eps = 1e-9;
    for (int j = 0; j < cols; ++j) {
      bool good = false;
      for (int i = 0; i < cols; ++i) {
        if (fabs(sigmas[i]) < eps && fabs(Vt[i+j*cols]) > eps) {
          good = true;
        }
      }
      if (!good) {
        new_dimensionless.push_back(j);
      }
    }

    if (new_dimensionless != dimensionless) { // maybe size difference at least 2 ?
      //      TRACE(rem_row _ new_dimensionless.size() _ dimensionless.size());
      bad_eqns.push_back(rem_row);
    }
  }

  delete[] A;
  delete[] sigmas;
  delete[] work;
  delete[] Vt;
}

void DimensionalAnalysis::instruction_opdecode(Instruction &inst) {
  int multiplier = 1;
  switch(inst.getOpcode()) {
    case Instruction::Add:
    case Instruction::FAdd:
    case Instruction::Sub:
    case Instruction::FSub:
    case Instruction::PHI:
      errs() << "Processing instruction: " << inst << '\n';
      for(Use &op : inst.operands())
        instruction_setequal(inst, *op);
      break;

    case Instruction::Mul:
    case Instruction::FMul:
    multiplier = -1;
    case Instruction::UDiv:
    case Instruction::SDiv:
    case Instruction::FDiv:
      errs() << "Processing instruction: " << inst << '\n';
      instruction_setadditive(inst, multiplier);
      break;

    case Instruction::Load:
      errs() << "Processing instruction: " << inst << '\n';
      insert_mem(*inst.getOperand(0));
      instruction_setequal(inst, *inst.getOperand(0), &DimensionalAnalysis::index_mem);
      break;

    case Instruction::Store:
      errs() << "Processing instruction: " << inst << '\n';
      insert_mem(*inst.getOperand(1));
      instruction_setequal(*inst.getOperand(1), *inst.getOperand(0), &DimensionalAnalysis::index_mem);
      break;

    case Instruction::GetElementPtr: {
      errs() << "Processing instruction: " << inst << '\n';
      if(insert_mem(inst) == -1)
        assert(false);
      break;
    }

    case Instruction::ICmp:
    case Instruction::FCmp:
      errs() << "Processing instruction: " << inst << '\n';
      instruction_setequal(*inst.getOperand(0), *inst.getOperand(1));
      break;
  }
}

void DimensionalAnalysis::instruction_setequal(const dimens_var &dest, const dimens_var &src) {
  instruction_setequal(dest, src, &DimensionalAnalysis::index);
}

void DimensionalAnalysis::instruction_setequal(const dimens_var &dest, const dimens_var &src,
    DimensionalAnalysis::index_type (DimensionalAnalysis::*indexer)(const dimens_var &)) {
  if(src.isa_constant() || dest.isa_constant())
    return;

  index_type d = (this->*indexer)(dest), s = (this->*indexer)(src);
  if(d == s)
    return;

  errs() << "\tdeg(" << (const string &) variables[d] << ") = deg(" << (const string &) variables[s] << ")\n";
  vector<int> equation;
  elem(equation, d) += 1;
  elem(equation, s) += -1;
  equations.push_back(move(equation));
}

void DimensionalAnalysis::instruction_setadditive(llvm::Instruction &line, int multiplier) {
  if(is_const(&line))
    return;

  bool ran = false;
  vector<int> equation;
  index_type lhs = index(line);
  elem(equation, lhs) += 1;
  for(Use &op : line.operands())
    if(!is_const(&*op)) {
      index_type term = index(*op);
      if(!ran) {
        // First term...
        errs() << "\tdeg(" << (const string &) variables[lhs] << ") = deg(" << (const string &) variables[term] << ')';
        // is always positive.
        elem(equation, term) += -1;
        ran = true;
      } else {
        // Subsequent term
        errs() << (multiplier < 0 ? " + " : " - ") << "deg(" << (const string &) variables[term] << ')';
        elem(equation, term) += multiplier;
      }
    }

  if(ran) {
    errs() << '\n';
    equations.push_back(move(equation));
  }
}

int &DimensionalAnalysis::elem(vector<int> &arr, DimensionalAnalysis::index_type idx) {
  assert(idx != -1);

  if(idx >= arr.size())
    arr.resize(idx + 1);
  return arr[idx];
}

DimensionalAnalysis::index_type DimensionalAnalysis::index_mem(const dimens_var &var) {
  return indirections.count(var) ? indirections[var] : index(var);
}

DimensionalAnalysis::index_type DimensionalAnalysis::insert_mem(Value &gep) {
  dimens_var noncanon = gep;
  index_type canonical = -1;

  if(GEPOperator *gep_oper = dyn_cast<GEPOperator>(&gep))
    if(PointerType *point = dyn_cast<PointerType>(gep_oper->getPointerOperandType())) {
      if(StructType *struct_ty = dyn_cast<StructType>(point->getElementType())) {
        DataLayout layout(module);
        IntegerType *point_ty = layout.getIntPtrType(module->getContext(), point->getAddressSpace());
        APInt offset(point_ty->getBitWidth(), 0);
        if(!gep_oper->accumulateConstantOffset(layout, offset))
          assert(false);
        canonical = index_mem(dimens_var(*struct_ty, offset));
      } else
        canonical = index_mem(*gep_oper->getOperand(0));
    }
  if(canonical == -1)
    return -1;

  if(!indirections.count(noncanon))
    indirections.emplace(noncanon, canonical);
  errs() << "\tindirect[" << (const string &) noncanon << "] = " << (const string &) variables[canonical] << '\n';
  return canonical;
}

DimensionalAnalysis::index_type DimensionalAnalysis::index(const dimens_var &var) {
  index_type res;
  if(indices.count(var))
    // There's already an entry for this program variable, so just use it.
    res = indices[var];
  else if(DIVariable *source_var = var)
    // This program variable is a register with an associated source variable, so use that.
    res = index(*source_var);
  else
    // This program variable is a new temporary we haven't seen before, so add an entry.
    res = insert(var);

  assert(res < variables.size());
  return res;
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
