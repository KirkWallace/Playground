# Playground
The playground has 4 major components: swingset, merry-go round, effects panel and touch pads. 
The code here converts the sensor data into midi triggers to use within ableton. The code is written in CPP because they are going onto arduinos. 

Since there are 4 major components you will see those in the major folders and then a series of additional folders that pertain to each component. 

In the Diagnostic_code directory you will find diagnostic code for each playground piece. For troubleshooting flash the board with this code. You can then view (in the Serial monitor) what midi signal the arduino thinks it is sending to the server. 

Steps to flash Arduino:
	
	1) Plug in arduino to be flashed (best to only have one arduino plugged in during this process)
	2) Open Arduino IDE from the applications folder
	3) Open the file you want to flash to the arduino (should match the playground piece you are looking to program)
	4) Select "Arduino Leonardo" from the Tools>Board menu drop down.
	5) Select the correct port for the board in Tools>Port (if you have multiple boards plugged in this will be hard to select which one you want to flash - so unplug any other boards at this time leaving only the board to be flashed)
		- on Windows this will look like COM1 or COM2 etc. 
		- on Mac this will likely look like /dev/tty.usbmodem241 or something like that
	6)click on the Arrow
	7)You should now see "compiling" then "uploading" then "done uploading" at the bottom of the IDE. The board should also flash when you do this step. Dont unplug the arduino until after you see "done uploading" in the IDE. 
	8)You are done! Use the arduino as expected now. Plug back in the other boards and reboot ableton to proceed. 
















