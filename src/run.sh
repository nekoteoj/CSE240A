#!/bin/bash

models=( "--tournament:9:10:10" "--gshare:13" "--custom" )
datas=( "fp_" "int_" "mm_" )
idxs=( 1 2 )

echo "" > result.txt

for data in "${datas[@]}"
do
    for idx in "${idxs[@]}"
    do
        echo "Testing " $data$idx >> result.txt
        for model in "${models[@]}"
        do
            echo "Model: " $model >> result.txt
            ./predictor $model ../traces/$data$idx >> result.txt
            echo "" >> result.txt
        done
    done
done