#!/bin/bash

set -euxo pipefail

SUFFIX=${1:-}

RUN_NAME=$(echo "$(hostname)$SUFFIX")

mkdir -p logs
exec > >(tee -a "logs/${RUN_NAME}-$(date +%d-%m-%Y-%s)") 2>&1


CORES=$(echo "scale=0; ($(lscpu | grep "Core" | cut -d: -f2 | xargs)*$(lscpu | grep "Socket" | cut -d: -f2 | xargs)) / 1" | bc)
P=$(echo "sqrt($CORES)" | bc)
while [ $P -gt 0 ]; do
    if [ $((CORES % P)) -eq 0 ]; then
        Q=$((CORES / P))
        break
    fi
    ((P--))
done

# N to use ~80 of the RAM
N=$(echo "scale=0; 0.8*sqrt($(free -b | awk '{print $2}' | sed "2q;d")/8) / 1" | bc)

echo "HPLinpack benchmark input file
Innovative Computing Laboratory, University of Tennessee
${RUN_NAME}.out      output file name (if any)
1            device out (6=stdout,7=stderr,file)
1            # of problems sizes (N)
${N}         Ns
3            # of NBs
64 128 256   NBs
1            PMAP process mapping (0=Row-,1=Column-major)
1            # of process grids (P x Q)
${P}         Ps
${Q}         Qs
16.0         threshold
1            # of panel fact
2            PFACTs (0=left, 1=Crout, 2=Right)
1            # of recursive stopping criterium
2            NBMINs (>= 1)
1            # of panels in recursion
2            NDIVs
1            # of recursive panel fact.
1            RFACTs (0=left, 1=Crout, 2=Right)
1            # of broadcast
0            BCASTs (0=1rg,1=1rM,2=2rg,3=2rM,4=Lng,5=LnM)
1            # of lookahead depth
0            DEPTHs (>=0)
0            SWAP (0=bin-exch,1=long,2=mix)
1            swapping threshold
1            L1 in (0=transposed,1=no-transposed) form
1            U  in (0=transposed,1=no-transposed) form
0            Equilibration (0=no,1=yes)
8            memory alignment in double (> 0)" > HPL.dat


echo "(specifications->manifest
  '(\"netlib-hpl\"
    \"openmpi\"
    \"coreutils\"))" > "${RUN_NAME}_manifest.scm"

# FIX: Get the exact version of mpi in guix that xhpl was compiled agains
# FIX 2: Get installation path of HPL previously since i am not in the enviroment to see the path
MPIRUN=$(guix gc -R $(guix time-machine -C channels.scm -- build netlib-hpl) | grep openmpi | head -n 1)/bin/mpirun

guix time-machine -C channels.scm -- shell --pure -m "${RUN_NAME}_manifest.scm" -- $MPIRUN --map-by core --bind-to core -np $CORES xhpl
