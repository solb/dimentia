#ifndef DIMENSIONAL_ANALYSIS_H_
#define DIMENSIONAL_ANALYSIS_H_

#include <llvm/Pass.h>
#include <unordered_map>
#include <vector>

class TraceVariablesNg;

struct dimens_var {
protected:
  unsigned long hash;
  std::string str;

  dimens_var(unsigned long hash, std::string &&str = "");

public:
  virtual ~dimens_var();
	bool operator==(const dimens_var &other) const;
	operator unsigned long() const;
  operator const std::string &() const;
};

namespace std {
template<>
struct hash<dimens_var> : hash<unsigned long> {};
}

class DimensionalAnalysis : public llvm::ModulePass {
private:
  typedef std::vector<dimens_var>::size_type index_type;

  std::vector<dimens_var> variables;
  std::unordered_map<dimens_var, index_type> indices;
  const TraceVariablesNg *groupings;

public:
  static char ID;

  DimensionalAnalysis();

  void getAnalysisUsage(llvm::AnalysisUsage &) const override;

  bool runOnModule(llvm::Module &) override;

  void print(llvm::raw_ostream &, const llvm::Module *) const override;

private:
  void insert(dimens_var &&);
};

#endif
