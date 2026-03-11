#!/bin/bash

set +euxo pipefail
date

PROJECT=${1:-}
if [[ -z $PROJECT ]]; then
    echo "Path for project file not informed."
    exit 1
fi

export GUIX_PROFILE="$HOME/.guix-profiles/simgrid-env"
source "$GUIX_PROFILE/etc/profile"
#eval $(guix time-machine -C channels.scm -- shell -m guix.scm --search-paths)

make clean
make pcad

BASEDIR="logs/SIM-$(basename -- "$PROJECT" .csv)-$(date +%d-%m-%y-%s)"
mkdir -p "$BASEDIR"

cp run-sim.sh create-pcad.cpp "$BASEDIR"

tail -n +2 "$PROJECT" | while IFS=, read -r PARTICAO NODOS NP APPL TAMANHO OMP B; do

    echo "[SMPI] Initializaion of simulation"

    NIVEL=$(echo "$B" | sed -E "s/.*([0-9]+)/\1/g")
    DIR="${BASEDIR}/${PARTICAO}-${NODOS}-${NP}-${APPL}-${TAMANHO}-${OMP}-${NIVEL}"

    mkdir -p "$DIR"

    echo "[SMPI] ${PARTICAO}-${NODOS}-${NP}-${APPL}-${TAMANHO}-${OMP}-${NIVEL}"

    HOSTFILE="$DIR/hostfile.txt"
    rm -f "$HOSTFILE"

    RANKS_PER_NODE=$(( NP / NODOS ))
    REMAINDER=$(( NP % NODOS ))

    for ((i=1; i<=NODOS; i++)); do
        RANKS=$RANKS_PER_NODE
        if (( i <= REMAINDER )); then
            RANKS=$((RANKS + 1))
        fi

        for ((j=0; j<RANKS; j++)); do
            echo "${PARTICAO}${i}/${j}" >> "$HOSTFILE"
        done
    done

    case "$APPL" in
    "lulesh")

        APP_DIR="./SMPI-proxy-apps/Benchmarks/Coral/Lulesh/"
        pushd $APP_DIR > /dev/null
        make clean > /dev/null
        make MPI_INC="" MPICXX="smpicxx -DUSE_MPI=1" -j 20 > /dev/null
        popd > /dev/null

        EXEC_BIN="${APP_DIR}/lulesh2.0 -i 100"

        case $TAMANHO in
        "p") PARAMETERS="-s 10" ;;
        "m") PARAMETERS="-s 40" ;;
        "g") PARAMETERS="-s 60" ;;
        esac
        ;;

    "minivite")

        APP_DIR="./SMPI-proxy-apps/Benchmarks/ECP/miniVite"
        pushd $APP_DIR > /dev/null
        make clean > /dev/null
        make CXX=smpicxx OPTFLAGS="-O3 -fopenmp -DPRINT_DIST_STATS -DDEBUG_PRINTF" -j 20 > /dev/null
        popd > /dev/null

        EXEC_BIN="${APP_DIR}/miniVite"
        case $TAMANHO in
        "p") PARAMETERS="-n 50000 -l" ;;
        "m") PARAMETERS="-n 200000 -l" ;;
        "g") PARAMETERS="-n 300000 -l" ;;
        esac
        ;;

    "comd")

        APP_DIR="./SMPI-proxy-apps/Benchmarks/ECP/CoMD"
        pushd $APP_DIR/src-mpi > /dev/null
        make clean > /dev/null
        make CC=smpicc -j 20 > /dev/null
        popd > /dev/null

        EXEC_BIN="${APP_DIR}/bin/CoMD-mpi -N 100"
	if [[ $NP == "8" ]]; then
	    EXEC_BIN="${APP_DIR}/bin/CoMD-mpi -N 100 -i 2 -j 2 -k 2"
	fi

        case $TAMANHO in
        "p") PARAMETERS="-x 10 -y 10 -z 10" ;;
        "m") PARAMETERS="-x 30 -y 30 -z 30" ;;
        "g") PARAMETERS="-x 50 -y 50 -z 50" ;;
        esac
        ;;

    *)
        echo "Unknown application: $APPL. Exiting..."
        exit 1
        ;;
    esac

    export OMP_NUM_THREADS=$OMP
    echo "[SMPI] Starting simulation..."
    TRACEFILE="$DIR/trace.rst"

    cat /sys/class/powercap/intel-rapl/intel-rapl\:*/energy_uj > "$DIR/start_uj"


    start=$(date +%s)

    smpirun -np $NP -trace -trace-file "$TRACEFILE" -hostfile "$HOSTFILE" -platform ./pcad.so --cfg=smpi/host-speed:21.125Gf $EXEC_BIN $PARAMETERS > "$DIR/smpi-$APPL.out"
    end=$(date +%s)

    cat /sys/class/powercap/intel-rapl/intel-rapl\:*/energy_uj > "$DIR/end_uj"

    echo "[SIM] END AT" $((end - start)) "s"


done

date
