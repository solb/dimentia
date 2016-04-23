#ifndef DIMENSIONAL_ANALYSIS_H_
#define DIMENSIONAL_ANALYSIS_H_

#include <llvm/Pass.h>
#include <unordered_map>
#include <vector>

namespace llvm {
class DIVariable;
class Instruction;
class Value;
}

class TraceVariablesNg;

struct dimens_var {
public:
  static const TraceVariablesNg *lookup;

private:
  unsigned long hash;
  std::string str;
  bool constant;
  llvm::DIVariable *svar;

  dimens_var(const void *hash, std::string &&str = "", bool constant = false);

public:
  dimens_var(const llvm::DIVariable &var);
  dimens_var(llvm::Value &var);
  virtual ~dimens_var();
	bool operator==(const dimens_var &other) const;
	operator unsigned long() const;
  operator const std::string &() const;

  // Corresponding source pointer, if different from this var itself.
  operator llvm::DIVariable *() const;
  bool isa_constant() const;
};

namespace std {
template<>
struct hash<dimens_var> : hash<unsigned long> {};
}

class DimensionalAnalysis : public llvm::ModulePass {
private:
  typedef std::vector<dimens_var>::size_type index_type;

  std::unordered_map<dimens_var, index_type> indirections;
  std::vector<dimens_var> variables;
  std::unordered_map<dimens_var, index_type> indices;
  std::vector<std::vector<int>> equations;
  std::vector<int> dimensionless;
  const TraceVariablesNg *groupings;

public:
  static char ID;

  DimensionalAnalysis();

  void getAnalysisUsage(llvm::AnalysisUsage &) const override;

  bool runOnModule(llvm::Module &) override;

  void print(llvm::raw_ostream &, const llvm::Module *) const override;

private:
  void calcDimensionless();

  void instruction_opdecode(llvm::Instruction &);
  void instruction_setequal(const dimens_var &dest, const dimens_var &src);
  void instruction_setequal(const dimens_var &dest, const dimens_var &src,
      index_type (DimensionalAnalysis::*indexer)(const dimens_var &));
  void instruction_setadditive(llvm::Instruction &line, int multiplier);

  int &elem(std::vector<int> &, index_type);
  index_type index_mem(const dimens_var &);
  index_type insert_mem(llvm::Value &);
  index_type index(const dimens_var &);
  index_type insert(const dimens_var &);
};

#endif
