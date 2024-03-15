benchmarks=$(patsubst %.cpp,%,$(wildcard *.cpp))
archs=native ivybridge westmere x86-64
CXXFLAGS=-g0 -O3 -std=gnu++2b -Wall -Wextra -Wno-psabi

stdsimd=$(shell test -f ../simd-prototyping/simd && echo '-include ../simd-prototyping/simd')
fastmath=-ffast-math
default=

variants=default fastmath
ifneq ($(stdsimd),)
variants+=stdsimd
endif

realtime::=chrt --fifo 10
realtime::=$(shell $(realtime) true 2>/dev/null && echo "$(realtime)")

all: all-targets

debug:
	@echo "$(realtime)"

bin:
	@mkdir -p bin

data:
	@mkdir -p data

targets=
define maketarget
bin/$1-$2-$3: $1.cpp bench.h bin bin/compile_commands.json
	$$(CXX) $$(CXXFLAGS) $$($2) -march=$3 -lmvec $1.cpp -o $$@

data/$1-$2-$3.out: bin/$1-$2-$3 data
	@./benchmark-mode.sh on
	@$$(realtime) $$< | tee $$@
	@./benchmark-mode.sh off

.PHONY: $1-$2-$3
$1-$2-$3: data/$1-$2-$3.out

targets+=$1-$2-$3

endef

$(foreach b,$(benchmarks),$(foreach v,$(variants),$(foreach a,$(archs),$(eval $(call maketarget,$b,$v,$a)))))

.PHONY: all-targets
all-targets: $(patsubst %,bin/%,$(targets))

.PHONY: benchmark
benchmark: $(targets)

define ccjson
  { "directory": "$(PWD)",
    "arguments": ["$(CXX)", $(2:%="%",) "-march=$1", "$3"],
    "file": "$3" },
endef

bin/compile_commands.json: Makefile run.sh bin
	@echo "Setting up 'compilation database' for Clang tooling."
	$(file >$@,[)
	$(foreach v,$(variants),$(foreach b,$(benchmarks),$(foreach a,$(archs),$(file >>$@,$(call ccjson,$a,$(CXXFLAGS) $($v),$b.cpp)))))
	@truncate --size=-2 "$@"
	@echo "" >> "$@"
	@echo "]" >> "$@"

help: bin/compile_commands.json
	@echo "$(targets)"|tr ' ' '\n'
	@echo "benchmark"
	@echo "all"
