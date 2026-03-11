#!/bin/bash
#SBATCH --job-name=platform-validation
#SBATCH --time=24:00:00
#SBATCH --output=logs/simgrid_validation_on_%N_%j.out
#SBATCH --error=logs/simgrid_validation_on_%N_%j.err

set +eux
date

#./run-sim.sh cei.csv
./run-sim-nodes.sh cei.csv


date
