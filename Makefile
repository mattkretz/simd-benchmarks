benchmarks=$(patsubst %.cpp,%,$(wildcard *.cpp))
archs=native ivybridge westmere k8
CXXFLAGS=-g0 -O2 -std=gnu++2b

define ccjson
  { "directory": "$(PWD)",
    "arguments": ["$(CXX)", $(CXXFLAGS:%="%",) "-march=$1", "$2"],
    "file": "$2" },
endef

bin/compile_commands.json: Makefile run.sh
	@echo "Setting up 'compilation database' for Clang tooling."
	@mkdir -p bin
	$(file >$@,[)
	$(foreach b,$(benchmarks),$(foreach a,$(archs),$(file >>$@,$(call ccjson,$a,$b.cpp))))
	@truncate --size=-2 "$@"
	@echo "" >> "$@"
	@echo "]" >> "$@"
