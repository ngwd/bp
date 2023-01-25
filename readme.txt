[server compiling]
g++ bp_server.cpp -std=c++11 -pthread  -o bp_server -g

[client compiling]
gcc bp_client.c -o bp_client -g

[ start server ]
./bp_server

[ start clients ]
for i in `seq 1 10`; do `nohup ./bp_client $((RANDOM)) &>> a.txt &`; done

NOTE, above commands will fork 10 processes of bp_client, each takes a ramdom number as argument, if argument is even, 
when running above command it shows
$ps -aux
hwd        161  0.0  0.0   2492   572 pts/3    S    20:35   0:00 ./bp_client 20334
hwd        163  0.0  0.0   2492   572 pts/3    S    20:35   0:00 ./bp_client 14415
hwd        165  0.0  0.0   2492   568 pts/3    S    20:35   0:00 ./bp_client 15096
hwd        169  0.0  0.0   2492   568 pts/3    S    20:35   0:00 ./bp_client 30837
hwd        171  0.0  0.0   2492   504 pts/3    S    20:35   0:00 ./bp_client 24214
hwd        173  0.0  0.0   2492   572 pts/3    S    20:35   0:00 ./bp_client 23090
hwd        175  0.0  0.0   2492   556 pts/3    S    20:35   0:00 ./bp_client 4368
hwd        177  0.0  0.0   2492   572 pts/3    S    20:35   0:00 ./bp_client 14187
hwd        179  0.0  0.0   2492   572 pts/3    S    20:35   0:00 ./bp_client 28572
hwd        181  0.0  0.0   2492   572 pts/3    S    20:35   0:00 ./bp_client 11600

it is a long task (task of 15s), else it is a short task. 
[test scenarion]

server and client both runs on WSL ubuntu 20.04

assume 10 processes above contains 5 processes arguments even, the running time of the this test is about 75s

I will still check how other async io libs, like netty, java nio or muduo, how they test this on one machine.

[existing problem ]
1. the bg_server pid is 127, check threads spawn by 127, it always shows 3 threads
$ ps -T -p 127
  PID  SPID TTY          TIME CMD
  127   127 pts/1    00:00:00 bp_server
  127   213 pts/1    00:00:00 bp_server
  127   214 pts/1    00:00:00 bp_server


[example log of client]
	socket(fd) 3 created
	connected
	17806 sent data :17806 (len 5 bytes) 
	17806 received data :17806 (len 5 bytes) 
	17806 closed 
	socket(fd) 3 created
	connected
	23163 sent data :23163 (len 5 bytes) 
	23163 received data :23163 (len 5 bytes) 
	23163 closed 
	socket(fd) 3 created
	connected
	23270 sent data :23270 (len 5 bytes) 
	23270 received data :23270 (len 5 bytes) 
	23270 closed 
	socket(fd) 3 created
	connected
	13254 sent data :13254 (len 5 bytes) 
	13254 received data :13254 (len 5 bytes) 
	13254 closed 
	socket(fd) 3 created
	connected
	17365 sent data :17365 (len 5 bytes) 
	17365 received data :17365 (len 5 bytes) 
	17365 closed 
	socket(fd) 3 created
	connected
	10704 sent data :10704 (len 5 bytes) 
	10704 received data :10704 (len 5 bytes) 
	10704 closed 
	socket(fd) 3 created
	connected
	8273 sent data :8273 (len 4 bytes) 
	8273 received data :8273 (len 4 bytes) 
	8273 closed 
	socket(fd) 3 created
	connected
	14021 sent data :14021 (len 5 bytes) 
	14021 received data :14021 (len 5 bytes) 
	14021 closed 
	socket(fd) 3 created
	connected
	25187 sent data :25187 (len 5 bytes) 
	25187 received data :25187 (len 5 bytes) 
	25187 closed 

[example log of server]
$ ./bp_server
Created a socket with fd: 3
listening on port 7000
new connection with fd: 4 accepted
data --> fd (4)
received data (len 5 bytes) to fd (4) from 20334
new connection with fd: 5 accepted
data --> fd (4)
close fd (4)
new connection with fd: 4 accepted
data --> fd (5)
received data (len 5 bytes) to fd (5) from 14415
new connection with fd: 6 accepted
data --> fd (4)
received data (len 5 bytes) to fd (4) from 15096
new connection with fd: 7 accepted
data --> fd (4)
close fd (4)
data --> fd (5)
close fd (5)
data --> fd (6)
received data (len 5 bytes) to fd (6) from 30837
new connection with fd: 4 accepted
data --> fd (7)
received data (len 5 bytes) to fd (7) from 24214
new connection with fd: 5 accepted
data --> fd (4)
received data (len 5 bytes) to fd (4) from 23090
data --> fd (6)
close fd (6)
data --> fd (7)
close fd (7)
new connection with fd: 6 accepted
data --> fd (4)
close fd (4)
data --> fd (5)
received data (len 4 bytes) to fd (5) from 4368
new connection with fd: 4 accepted
data --> fd (5)
close fd (5)
data --> fd (6)
received data (len 5 bytes) to fd (6) from 11600
new connection with fd: 5 accepted
data --> fd (4)
received data (len 5 bytes) to fd (4) from 14187
data --> fd (6)
close fd (6)
data --> fd (4)
close fd (4)
data --> fd (5)
received data (len 5 bytes) to fd (5) from 28572
data --> fd (5)
close fd (5)






Thread creation is a relatively long operation. 
When the number of active (working) threads becomes large, 
the context switch between them starts taking significant time and becomes comparable or even bigger than a user time. 
That is why usually few (10) threads are created ahead and the threads live in a pool waiting 
till an object of a work comes. 
The work objects are usually placed into a queue. 
So the threads wait and then pull the objects from the queue. 
When a thread finishes handling a work object it returns back to the thread pool and waits till the new work object appears in the queue. Etc.
