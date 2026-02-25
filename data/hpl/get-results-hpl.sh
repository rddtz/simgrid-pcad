 echo "machine,t/v,n,nb,p,q,time,gflops"; for d in *-hpl-*; do cat $d/*.out | grep WC00 | sed "s/WC00/$(echo "$d" | cut -d- -f1) WC00"/g | sed "s/$/RARA/g" | xargs | sed "s/RARA/\n/g" | sed "s/^ //g" | sed "s/ /,/g" | sed "/^$/d" ; done; 

