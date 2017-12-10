#!/bin/bash

python copyid.py &&
python deploy.py &&
python start.py &&
python test.py
