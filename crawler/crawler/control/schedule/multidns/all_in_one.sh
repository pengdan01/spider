#!/bin/bash

mkdir -p output tmp validate error merge
bash -x clean.sh
python run.py &&
python merge.py &&
python validate.py

