#!/bin/sh

LSAN_OPTIONS=suppressions=./lsan_supp.txt  ./build/$1
