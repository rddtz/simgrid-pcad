#!/bin/bash
#SBATCH --job-name=Real_minivite_2
#SBATCH --partition=poti
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --output=logs/REAL-proj_fim-24-02-26-1771942383/REAL-poti-1-1-minivite-m-1-2/exec_real.out
#SBATCH --error=logs/REAL-proj_fim-24-02-26-1771942383/REAL-poti-1-1-minivite-m-1-2/exec_real.err
#SBATCH --time=01:00:00

# Carrega o ambiente no nó de execução
export GUIX_PROFILE="/home/users/rrdmatos/.guix-profiles/simgrid-env"
source "/home/users/rrdmatos/.guix-profiles/simgrid-env/etc/profile"
#eval $(guix time-machine -C channels.scm -- shell -m guix.scm --search-paths)

export OMP_NUM_THREADS=1

echo "[REAL] Starting execution..."

# CORREÇÃO: O srun vai espalhar 8 tarefas e cada uma vai escrever
# onde está rodando. Isso cria um arquivo com exatamente 8 linhas!
srun hostname | sort > logs/REAL-proj_fim-24-02-26-1771942383/REAL-poti-1-1-minivite-m-1-2/hosts.txt

# Medição de Energia Inicial (Distribuída por todos os nós alocados pelo Slurm)
srun --nodes=1 --ntasks=1 bash -c "hostname; cat /sys/class/powercap/intel-rapl/intel-rapl\:*/energy_uj"  > "logs/REAL-proj_fim-24-02-26-1771942383/REAL-poti-1-1-minivite-m-1-2/start_uj"

# O mpirun nativo lê a alocação do Slurm automaticamente (sem necessidade de hostfile)
mpirun -np 1 -hostfile logs/REAL-proj_fim-24-02-26-1771942383/REAL-poti-1-1-minivite-m-1-2/hosts.txt ./SMPI-proxy-apps-real/Benchmarks/ECP/miniVite/miniVite -n 200000 -l > "logs/REAL-proj_fim-24-02-26-1771942383/REAL-poti-1-1-minivite-m-1-2/minivite.out"

# Medição de Energia Final
srun --nodes=1 --ntasks=1 bash -c "hostname; cat /sys/class/powercap/intel-rapl/intel-rapl\:*/energy_uj"  > "logs/REAL-proj_fim-24-02-26-1771942383/REAL-poti-1-1-minivite-m-1-2/end_uj"

echo "[REAL] Execution finished."
