# MemoryMonitor

A Linux software that assists developers in monitoring various memory addresses during runtime.
The software will print the value of the requested memory address every time it is modified.
As this program is independent of the software being monitored, there is no need to add random print statements, which also makes your code cleaner.


## How to compile
Because it only one file I didn't made a Makefile
```bash
g++ MemoryMonitorLinux.cpp -o Monitor
```

## How to use
```text
This monitor needs at least 3 parameters:
Example:
    # sudo ./Monitor 246343 d_3ffb260
    (if root) # ./Monitor 246343 d_3ffb260
First parameter needs to be the process id
Parameters to monitor (3rd and previous parameters) needs to start with the type and continue with the address of the memory
Type list:
b: bool
d: double
f: float
i: int
c: char
ui: unsigned int
...
```
