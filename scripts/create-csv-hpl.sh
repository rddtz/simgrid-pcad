#!/bin/bash

# Criar o cabeçalho do CSV
echo "Machine,Run_ID,N,NB,P,Q,Time,GFlops" > hpl-results.csv

# Localizar todos os arquivos .out recursivamente
find . -type f -name "*.out" | while read -r file; do
    filename=$(basename "$file")

    # 1. Extrair Machine e Run_ID do nome do arquivo
    # Ex: poti31769806285.out -> Machine: poti3, Run_ID: 1769806285
    # Captura letras e o primeiro número (ex: cei1, draco2, poti3)
    machine=$(echo "$filename" | sed -r 's/^([a-zA-Z]+[0-9]).*/\1/')

    # Captura apenas a sequência longa de números antes do .out
    run_id=$(echo "$filename" | sed -r 's/^[a-zA-Z]+[0-9]([0-9]+)\.out/\1/')

    # 2. Ler todas as linhas de resultado (que começam com WR)
    grep "^WC" "$file" | while read -r line; do
        # O HPL organiza as colunas como: T/V, N, NB, P, Q, Time, Gflops
        n=$(echo "$line" | awk '{print $2}')
        nb=$(echo "$line" | awk '{print $3}')
        p=$(echo "$line" | awk '{print $4}')
        q=$(echo "$line" | awk '{print $5}')
        time=$(echo "$line" | awk '{print $6}')
        gflops=$(echo "$line" | awk '{print $7}')

        # Salvar no CSV
        echo "${machine},${run_id},${n},${nb},${p},${q},${time},${gflops}" >> hpl-results.csv
    done
done
