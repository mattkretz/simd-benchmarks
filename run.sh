#!/bin/bash

usage() {
  cat <<EOF
Usage: $0 <name> [<compiler -f -I or -D flags>] [<arch list>]

<name> must be one of: $(echo *.cpp|sed 's/.cpp\>//g')
EOF
}

name=$1
flags=()
shift
while (($# > 0)); do
    case "$1" in
        -f*)
            flags=(${flags[@]} $1)
            ;;
        -[ID])
            flags=(${flags[@]} $1$2)
            shift
            ;;
        -[ID]*)
            flags=(${flags[@]} $1)
            ;;
        *)
            arch_list="$arch_list $1"
            ;;
    esac
    shift
done
test -z "$arch_list" && arch_list="native westmere k8"

if ! test -r ${name}.cpp; then
  name=${name%.cpp}
  if ! test -r ${name}.cpp; then
    usage
    exit 1
  fi
fi

#CCACHE=`which ccache 2>/dev/null` || CCACHE=

mkdir -p bin
${0%/*}/benchmark-mode.sh on
for arch in ${arch_list}; do
  CXXFLAGS="-g0 -O3 -std=gnu++20 -march=$arch -lmvec"

  echo $CXX $CXXFLAGS ${flags[@]} ${name}.cpp -o "bin/$name-$arch"
  ccache $CXX $CXXFLAGS ${flags[@]} ${name}.cpp -o "bin/$name-$arch" && \
    echo "-march=$arch $flags:" && \
    sudo chrt --fifo 50 "bin/$name-$arch"
done
${0%/*}/benchmark-mode.sh off

# vim: tw=0 si
