#!/usr/bin/env bash

make
mkdir -p output/conf
cp src/predixy output/predixy
cp conf/* output/conf