make
fld=$1
n=$2
p=$3

mkdir -p output
for ((i=1 ; i<=$n ; i++))
do
	timeout 60 ./client-phase$p ./$fld/client$i-config.txt ./$fld/files/client$i/ > ./output/op-c$i-p$p.txt &
done

sleep 2
rm -fr diffs
mkdir -p diffs
for ((i=1 ; i<=$n ; i++))
do
	diff -Bw ./output/op-c$i-p$p.txt ./sol4_0/ans_$i\_$p.txt > ./diffs/comp-c$i-p$p.txt
done