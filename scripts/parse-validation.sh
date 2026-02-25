BASE=${1:-}

if [[ "$BASE" == "" ]]; then
    BASE="."
fi

echo "partition,nodes,np,app,size,omp,run,time,start_uj,end_uj";

for i in `find $BASE -name *lulesh* -type d`; do

    config=$(echo $i | rev | cut -d/ -f1 | rev | sed s/-/,/g | sed s/REAL,//g)

    empty=$(find $i -name "*_uj" | wc -l)

    if [[ "$empty" == "2" ]]; then

	time=$(cat $i/*.out | grep "Elapsed time" | sed -E "s/.* ([0-9]+.[0-9]+) .*/\1/g")

	start_uj=$(cat $i/start_uj | awk '{s+=$1} END {print s}')
	end_uj=$(cat $i/end_uj | awk '{s+=$1} END {print s}')

	echo $config,$time,$start_uj,$end_uj

    fi


done


for i in `find $BASE -name *minivite* -type d`; do
    config=$(echo $i | rev | cut -d/ -f1 | rev | sed s/-/,/g | sed s/REAL,//g)

    empty=$(find $i -name "*_uj" | wc -l)

    if [[ "$empty" == "2" ]]; then

    time=$(cat $i/*.out | grep "Average total time" | sed -E "s/.* ([0-9]+.[0-9]+),.*/\1/g" | xargs)

    start_uj=$(cat $i/start_uj | awk '{s+=$1} END {print s}')
    end_uj=$(cat $i/end_uj | awk '{s+=$1} END {print s}')

    echo $config,$time,$start_uj,$end_uj

    fi

done

for i in `find $BASE -name *comd* -type d`; do
    config=$(echo $i | rev | cut -d/ -f1 | rev | sed s/-/,/g  | sed s/REAL,//g)

    empty=$(find $i -name "*_uj" | wc -l)

    if [[ "$empty" == "2" ]]; then

    time=$(cat $i/*.out | grep "^total" | tail -n 1 | awk '{print $6}')

    start_uj=$(cat $i/start_uj | awk '{s+=$1} END {print s}')
    end_uj=$(cat $i/end_uj | awk '{s+=$1} END {print s}')

    echo $config,$time,$start_uj,$end_uj

    fi
done
