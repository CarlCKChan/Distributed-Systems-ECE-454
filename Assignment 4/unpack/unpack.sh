#!/bin/bash
export mark=0
set -e
rm -rf ece454a4
unzip ece454a4.zip
cd ece454a4
rm -rf ./tmp
make clean
make server

#Note: the following line should create libclient-api.a
make client-api.a 

./server . >> ./tmp &
sleep 1s
srvout=$(cat ./tmp)
echo $srvout
rm -rf ./tmp
IFS=' ' read ip port <<< "$srvout"
echo "ip=${ip}, port=${port}"
cd ..
rm -rf fs_client fs_client1
gcc -o fs_client fs_client.c -Lece454a4 -lclient-api
./fs_client $ip $port
export mark=30
# more complex test
gcc -o fs_client1 fs_client1.c -Lece454a4 -lclient-api
./fs_client1 $ip $port
export mark=90
rm -rf fs_client fs_client1
