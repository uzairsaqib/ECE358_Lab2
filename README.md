# ECE 358
## Lab 02

The purpose of this code is to simulate persistent and non-persistent sensing CSMA/CD.

### Setup
- Since the LEARN dropdox does not accept `.c`, `.h`, and `.zip` files, each file name has been augmented with a `.txt` extension
- To setup the simulator, ensure that all the files (including the makefile) are in the same directory and remove the `.txt`
  extension from each file (including the makefile)

### How to Compile and Run the Simulator
- From the terminal, navigate to the directory where this README is found
- Type `make` into the console (this will build the simulator)
- To the run the simulator, type `./queueSim`

### How to Switch Between Simulation Modes
- In `app_simulator.c` on `line 22`, set the value of the define switch labelled `SIMULATE_PERSISTENT_CASE`
  to `1U` to simulate the persistent case, or `0U` to simulate the non-persistent case
