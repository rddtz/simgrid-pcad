#!/bin/bash
#SBATCH --job-name=HPL
#SBATCH --nodes=1
#SBATCH --time=10:00:00
#SBATCH --output=%N_%x_%j.out
#SBATCH --error=%N_%x_%j.err

set +euxo pipefail

date +%D-%T 2>&1

OUTDIR="`hostname`-hpl-`date +%d-%m-%Y-%s`"

mkdir $OUTDIR

cp ./run-hpl.sh channels.scm $OUTDIR

pushd $OUTDIR
{
for i in `seq 1 3`; do

    ./run-hpl.sh `date +%s`

done
}
popd

date +%D-%T 2>&1
