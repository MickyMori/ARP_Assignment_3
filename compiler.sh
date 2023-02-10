#Create bin directory
mkdir -p bin &

#create out directory
mkdir -p out &

#create logFiles directory
mkdir -p logFiles &

#Compile the process A
gcc src/processA.c -lncurses -lbmp -lm -o bin/processA &

#Compile the process B
gcc src/processB.c -lncurses -lbmp -lm -o bin/processB &

#Compile the master process
gcc src/master.c -o bin/master &


