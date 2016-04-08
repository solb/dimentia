LLVERSION := 3.7

CLEAN := git clean
CXX := clang-$(LLVERSION)
CXXFLAGS := $(shell llvm-config-$(LLVERSION) --cxxflags) -UNDEBUG
LDFLAGS := $(shell llvm-config-$(LLVERSION) --ldflags)

.PHONY: all
all: GenerateEquations.so TraceVariables.so

.PHONY: clean
clean:
	$(CLEAN) -fX

GenerateEquations.o: GenerateEquations.h TraceVariables.h
TraceVariables.o: TraceVariables.h

%.so: %.o
	$(CXX) -shared $(LDFLAGS) $^ $(LDLIBS) -o $@
