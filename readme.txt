[server compiling]
g++ bp_server.cpp -std=c++11 -pthread bp_server.cpp  -o bp_server -g

[client compiling]
gcc bp_client.c -o bp_client

[ start server ]
./bp_server

[ start clients ]

// for i in `seq 1 30`; do `./bp_client $((RANDOM)) &> a.txt`; done
// for i in `seq 1 30`; do `nohup ./bp_client $((RANDOM)) &> a.txt&`; done

for i in `seq 1 300`; do `nohup ./bp_client $((RANDOM)) &>> a.txt &`; done




