#!/bin/bash

python deploy.py &&
python stop.py &&
python start.py &&
python test.py
