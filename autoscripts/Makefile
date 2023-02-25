all: build1 build2 build3

run1:
	./../ns3 run "first-bulksend --pathOut=./auto/pi/raw"
run2:
	./../ns3 run "second-bulksend --pathOut=./auto/pi/raw"
run3:
	./../ns3 run "third-mix --pathOut=./auto/pi/raw"

plot1:
	./pi/pi-queue1
plot2:
	./pi/pi-queue2	
plot3:
	./pi/pi-queue3
	
build1: run1 plot1
build2: run2 plot2
build3: run3 plot3

