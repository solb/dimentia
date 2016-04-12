LLVERSION := 3.7

CLEAN := git clean
CXX := clang-$(LLVERSION)
CXXFLAGS := $(shell llvm-config-$(LLVERSION) --cxxflags) -UNDEBUG
LDFLAGS := $(shell llvm-config-$(LLVERSION) --ldflags)
LDFLAGS := $(LDFLAGS) -lblas -llapack

SOFILE=build/DbgPass.so

.PHONY: all
all: $(SOFILE)

.PHONY: clean
clean:
	rm -rf build
	$(CLEAN) -fX

build/%.so: %.o
	mkdir -p $(@D)
	$(CXX) -shared $(LDFLAGS) $^ $(LDLIBS) -o $@

run::
	opt-$(LLVERSION) -load $(SOFILE) -my-dbg ../../tests/annot0.ll -o build/tmp
