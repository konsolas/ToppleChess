#!/bin/bash -xe

cpus=$(nproc)
threads=1
concurrency=64
cutechess_path=~/cutechess-cli
syzygy_path=~/syzygy
book_file=~/openings.pgn
book_format=pgn;
time_control="60+1"

commit_1=$1
commit_2=$2

git fetch
mkdir -p build

git checkout -- .
git checkout "$commit_1"
read -pr "$commit_1: press enter to continue: "
cd build
cmake ..
make -j "$cpus" Topple
mv Topple "$commit_1"
cd ..

git checkout -- .
git checkout "$commit_2"
read -pr "$commit_2: press enter to continue: "
cd build
cmake ..
make -j "$cpus" Topple
mv Topple "$commit_2"
cd ..

mkdir -p test
cd test

$cutechess_path -openings file=$book_file format=$book_format order=sequential -repeat \
    -each dir=. option.Hash=512 option.Threads=$threads restart=on stderr=cutechess.log proto=uci tc=$time_control \
    -engine name="$commit_1" cmd=../build/"$commit_1" -engine name="$commit_2" cmd=../build/"$commit_2" \
    -concurrency $concurrency -games 10000 \
    -sprt elo0=0 elo1=5 alpha=0.1 beta=0.1 \
    -resign movecount=5 score=1000 \
    -draw movenumber=35 movecount=10 score=5 \
    -tb $syzygy_path
