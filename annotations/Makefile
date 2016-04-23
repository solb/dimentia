LLVERSION := 3.7

CLEAN := git clean
CXX := clang-$(LLVERSION)
CXXFLAGS := $(shell llvm-config-$(LLVERSION) --cxxflags) -UNDEBUG
LDFLAGS := $(shell llvm-config-$(LLVERSION) --ldflags)
LDFLAGS := $(LDFLAGS) -lblas -llapack

.PHONY: all
all: DimensionalAnalysis.so TraceVariablesNg.so

.PHONY: clean
clean:
	$(CLEAN) -fX

DimensionalAnalysis.o: DimensionalAnalysis.h TraceVariablesNg.h
TraceVariablesNg.o: TraceVariablesNg.h

%.so: %.o
	$(CXX) -shared $(LDFLAGS) $^ $(LDLIBS) -o $@
