echo "client,server,mean_speed_mega"

for f in *calibracao*; do
    SER=$(cat $f | grep Server: | cut -d: -f2 | xargs)

    CLI=$(cat $f | grep Client: | cut -d: -f2 | xargs)

    cat $f | grep sender | \
    sed -E "s/.* ([0-9.]+ .)bits.*/\1/g" | \
    sed "s/ M//g" | sed "s/ G/\*1000/g" |\
    bc -l | sed "s/^/$CLI,$SER,/g"
done
