SHELL := bash

all: build1 build2 build3

run1:
	rm -f ./pi/raw/pi-queue1*
	for tcp in TcpCubic TcpNewReno TcpBic TcpLinuxReno; do \
		./../ns3 run "first-bulksend --pathOut=./autoscripts/pi/raw --tcpType=$${tcp}"; \
	done
run2:
	./../ns3 run "second-bulksend --pathOut=./autoscripts/pi/raw"
run3:
	./../ns3 run "third-mix --pathOut=./autoscripts/pi/raw"

plot1:
	rm -f ./pi/result/pi-queue1*
	for tcp in TcpCubic TcpNewReno TcpBic TcpLinuxReno; do \
		gnuplot -c ./pi/pi-queue1 $${tcp}; \
	done
plot2:
	./pi/pi-queue2	
plot3:
	./pi/pi-queue3
	
build1: run1 plot1
build2: run2 plot2
build3: run3 plot3

