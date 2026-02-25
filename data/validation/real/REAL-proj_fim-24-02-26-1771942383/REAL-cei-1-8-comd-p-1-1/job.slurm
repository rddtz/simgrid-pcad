#!/bin/bash
#SBATCH --job-name=Real_comd_1
#SBATCH --partition=cei
#SBATCH --nodes=1
#SBATCH --ntasks=8
#SBATCH --output=logs/REAL-proj_fim-24-02-26-1771942383/REAL-cei-1-8-comd-p-1-1/exec_real.out
#SBATCH --error=logs/REAL-proj_fim-24-02-26-1771942383/REAL-cei-1-8-comd-p-1-1/exec_real.err
#SBATCH --time=01:00:00

# Carrega o ambiente no nó de execução
export GUIX_PROFILE="/home/users/rrdmatos/.guix-profiles/simgrid-env"
source "/home/users/rrdmatos/.guix-profiles/simgrid-env/etc/profile"
#eval $(guix time-machine -C channels.scm -- shell -m guix.scm --search-paths)

export OMP_NUM_THREADS=1

echo "[REAL] Starting execution..."

# CORREÇÃO: O srun vai espalhar 8 tarefas e cada uma vai escrever
# onde está rodando. Isso cria um arquivo com exatamente 8 linhas!
srun hostname | sort > logs/REAL-proj_fim-24-02-26-1771942383/REAL-cei-1-8-comd-p-1-1/hosts.txt

# Medição de Energia Inicial (Distribuída por todos os nós alocados pelo Slurm)
srun --nodes=1 --ntasks=1 bash -c "hostname; cat /sys/class/powercap/intel-rapl/intel-rapl\:*/energy_uj"  > "logs/REAL-proj_fim-24-02-26-1771942383/REAL-cei-1-8-comd-p-1-1/start_uj"

# O mpirun nativo lê a alocação do Slurm automaticamente (sem necessidade de hostfile)
mpirun -np 8 -hostfile logs/REAL-proj_fim-24-02-26-1771942383/REAL-cei-1-8-comd-p-1-1/hosts.txt ./SMPI-proxy-apps-real/Benchmarks/ECP/CoMD/bin/CoMD-mpi -i 2 -j 2 -k 2 -N 100 -x 10 -y 10 -z 10 > "logs/REAL-proj_fim-24-02-26-1771942383/REAL-cei-1-8-comd-p-1-1/comd.out"

# Medição de Energia Final
srun --nodes=1 --ntasks=1 bash -c "hostname; cat /sys/class/powercap/intel-rapl/intel-rapl\:*/energy_uj"  > "logs/REAL-proj_fim-24-02-26-1771942383/REAL-cei-1-8-comd-p-1-1/end_uj"

echo "[REAL] Execution finished."
