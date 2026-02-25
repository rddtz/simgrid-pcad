echo "from,to,time"

for f in *_ping_*; do

    NODES=$(cat $f | grep Origem |\
		sed -E "s/.*: ([a-z]+[0-9]).*: ([a-z]+[0-9])/\1\n\2/g")

    ALMOST=$(cat $f | grep -A100 PING |\
		 sed -E "s/.*( [a-z]+[0-9] ).*time=([0-9.]+).*/\1,\2/g" |\
		 sed "s/ //g" | grep ,)

    echo "$NODES" | while read name; do

	echo "$ALMOST" | grep -v $name |\
	    sed -E "s/^/$name,/g"
    done
 done
