This directory was created to automatically add the PI algorithm to an NS-3 version 3.36.1 project.
To automatically add everything you need to the project, you need to place this directory in the NS-3 root, namely the ns-3.36.1 directory (maybe a slightly different version), then you should go into this directory and write the "./automodify" command. This command will run the automodify file, which will automatically add the PI algorithm to the project and 3 traffic generation files that are needed to simulate various situations of working out our algorithm. After a successful launch, this directory can be removed from the ns-3.36.1 directory.
To automatically launch traffic files, you need to go to the expanded autoscripts directory and enter the make command, after which 3 traffic files will be launched one by one and graphs will be built based on the results obtained. All results will end up in the autoscripts/pi/result directory.
If you want to run the files manually, then you need to write "./ns3 run <filename>", as an example "./ns3 run first-bulksend".
