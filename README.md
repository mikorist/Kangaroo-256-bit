# Pollard's kangaroo for SECPK1

This 256bit version is based on:

https://github.com/JeanLucPons/Kangaroo        125bit version

With ideas from:

https://github.com/ZenulAbidin/Kangaroo-256    (said to be buggy and not finished)

https://github.com/Totulik/Kangaroo-254-bit

https://github.com/AlberTajuelo/kangaroo

A Pollard's kangaroo interval ECDLP solver for SECP256K1 (based on VanitySearch engine).

# Feature

<ul>
  <li>Fixed size arithmetic.</li>
  <li>Fast Modular Inversion (Delayed Right Shift 62 bits).</li>
  <li>SecpK1 Fast modular multiplication (2 steps folding 512bits to 256bits reduction using 64 bits digits).</li>
  <li>Multi-GPU support.</li>
  <li>CUDA optimisation via inline PTX assembly.</li>
  <li>Full 256-bit interval search.</li>
</ul>

# Discussion Thread

Discusion thread: https://bitcointalk.org/index.php?topic=5244940.0

# Usage

```
Kangaroo v2.3
Kangaroo [-v] [-t nbThread] [-d dpBit] [gpu] [-check]
         [-gpuId gpuId1[,gpuId2,...]] [-g g1x,g1y[,g2x,g2y,...]]
         inFile
 -v: Print version
 -gpu: Enable gpu calculation
 -gpuId gpuId1,gpuId2,...: List of GPU(s) to use, default is 0
 -g g1x,g1y,g2x,g2y,...: Specify GPU(s) kernel gridsize, default is 2*(MP),2*(Core/MP)
 -d: Specify number of leading zeros for the DP method (default is auto)
 -t nbThread: Secify number of thread
 -w workfile: Specify file to save work into (current processed key only)
 -i workfile: Specify file to load work from (current processed key only)
 -wi workInterval: Periodic interval (in seconds) for saving work
 -ws: Save kangaroos in the work file
 -wss: Save kangaroos via the server
 -wsplit: Split work file of server and reset hashtable
 -wm file1 file2 destfile: Merge work file
 -wmdir dir destfile: Merge directory of work files
 -wt timeout: Save work timeout in millisec (default is 3000ms)
 -winfo file1: Work file info file
 -wpartcreate name: Create empty partitioned work file (name is a directory)
 -wcheck worfile: Check workfile integrity
 -m maxStep: number of operations before give up the search (maxStep*expected operation)
 -s: Start in server mode
 -c server_ip: Start in client mode and connect to server server_ip
 -sp port: Server port, default is 17403
 -nt timeout: Network timeout in millisec (default is 3000ms)
 -o fileName: output result to fileName
 -l: List cuda enabled devices
 -check: Check GPU kernel vs CPU
 inFile: intput configuration file
```

Structure of the input file:
* All values are in hex format
* Public keys can be given either in compressed or uncompressed format

```
Start range
End range
Key #1
```

ex

```
200000000000000000000000000000000
3FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
03633CBE3EC02B9401C5EFFA144C5B4D22F87940259634858FC7E59B1C09937852
```

# Note on Time/Memory tradeoff of the DP method

The distinguished point (DP) method is an efficient method for storing random walks and detect collision between them. Instead of storing all points of all kangagroo's random walks, we store only points that have an x value starting with dpBit zero bits. When 2 kangaroos collide, they will then follow the same path because their jumps are a function of their x values. The collision will be then detected when the 2 kangaroos reach a distinguished point.\
This has a drawback when you have a lot of kangaroos and looking for collision in a small range as the overhead is in the order of nbKangaroo.2<sup>dpBit</sup> until a collision is detected. If dpBit is too small a large number of point will enter in the central table, will decrease performance and quickly fill the RAM.
**Powerfull GPUs with large number of cores won't be very efficient on small range, you can try to decrease the grid size in order to have less kangaroos but the GPU performance may not be optimal.**
Yau can change manualy the DP mask size using the -d option, take in consideration that it will require more operations to complete. See table below:

| nbKangaroo.2<sup>dpBit</sup>/sqrt(N) |  DP Overhead | Avg | 
|--------------------------------------|:------------:|:---:|
| 4.000 | cubicroot(1+4.000) = ~71.0% | 3.55 sqrt(N) |
| 2.000 | cubicroot(1+2.000) = ~44.2% | 2.99 sqrt(N) |
| 1.000 | cubicroot(1+1.000) = ~26.0% | 2.62 sqrt(N) |
| 0.500 | cubicroot(1+0.500) = ~14.5% | 2.38 sqrt(N) |
| 0.250 | cubicroot(1+0.250) = ~7.7% | 2.24 sqrt(N) |
| 0.125 | cubicroot(1+0.125) = ~4.0% | 2.16 sqrt(N) |

DP overhead according to the range size (N), DP mask size (dpBit) and number of kangaroos running in paralell (nbKangaroo).

# Probability of success

The picture below show the probability of success after a certain number of group operations. N is range size.
This plot does not take into consideration the DP overhead.

![Probability of success](DOC/successprob.jpg)


# How it works

The program uses 2 herds of kangaroos, a tame herd and a wild herd. When 2 kangoroos (a wild one and a tame one) collide, the 
key can be solved. Due to the birthday paradox, a collision happens (in average) after 2.08*sqrt(k2-k1) [1] group operations, the 2 herds have the same size. To detect collision, the distinguished points method is used with a hashtable.

Here is a brief description of the algorithm:

We have to solve P = k.G, P is the public key, we know that k lies in the range [k1,k2], G is the SecpK1 generator point.\
Group operations are additions on the elliptic curve, scalar operations are done modulo the order of the curve.

n = floor(log2(sqrt(k2-k1)))+1

* Create a jump point table jP = [G,2G,4G,8G,...,2<sup>n-1</sup>.G]
* Create a jump distance table jD = [1,2,4,8,....,2<sup>n-1</sup>]
 
for all i in herdSize</br>
&nbsp;&nbsp;tame<sub>i</sub> = rand(0..(k2-k1)) <em># Scalar operation</em></br>
&nbsp;&nbsp;tamePos<sub>i</sub> = (k1+tame<sub>i</sub>).G <em># Group operation</em></br>
&nbsp;&nbsp;wild<sub>i</sub> = rand(0..(k2-k1)) - (k2-k1)/2 <em># Scalar operation</em></br>
&nbsp;&nbsp;wildPos<sub>i</sub> = P + wild<sub>i</sub>.G <em># Group operation</em></br>

found = false</br>

while not found</br>
&nbsp;&nbsp;for all i in herdSize</br>
&nbsp;&nbsp;&nbsp;&nbsp;  tamePos<sub>i</sub> = tamePos<sub>i</sub> + jP[tamePos<sub>i</sub>.x % n] <em># Group operation</em></br>
&nbsp;&nbsp;&nbsp;&nbsp;  tame<sub>i</sub> += jD[tamePos<sub>i</sub>.x % n] <em># Scalar operation</em></br>
&nbsp;&nbsp;&nbsp;&nbsp;  wildPos<sub>i</sub> = wildPos<sub>i</sub> + jP[wildPos<sub>i</sub>.x % n] <em># Group operation</em></br>
&nbsp;&nbsp;&nbsp;&nbsp;  wild<sub>i</sub> += jD[wildPos<sub>i</sub>.x % n] <em># Scalar operation</em></br>
&nbsp;&nbsp;&nbsp;&nbsp;  if tamePos<sub>i</sub> is distinguished</br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  add (TAME,tamePos<sub>i</sub>,tame<sub>i</sub>) to hashTable</br>
&nbsp;&nbsp;&nbsp;&nbsp;  if wildPos<sub>i</sub> is distinguished</br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;  add (WILD,wildPos<sub>i</sub>,wild<sub>i</sub>) to hashTable</br>
&nbsp;&nbsp;found = is there a collision in hashTable between a tame and a wild kangaroo ?</br>
</br>

(Tame,Wild) = Collision</br>
k = k1 + Tame.dist - Wild.dist</br>


Here is an illustration of what's happening. When 2 paths collide they form a shape similar to the lambda letter. This is why this method is also called lambda method.

![Paths](DOC/paths.jpg)

# Compilation

Compilation Commands:
```
make gpu=1    Compile with GPU support.

make cpu=1    Compile optimized for CPUs.

make debug=1  Compile with debug symbols for debugging.
```

Main Target: Compiles the final executable kangaroo-256.

Dependency Management: Ensures that necessary directories ($(OBJDIR), $(OBJDIR)/SECPK1, $(OBJDIR)/GPU) are created before compiling.

Cleaning: Removes all object files and other generated files (cuda_version.txt, deviceQuery/cuda_build_log.txt) when make clean is invoked.

This Makefile structure allows you to easily switch between different compilation configurations (amd, gpu, default) by setting appropriate flags when invoking make, or by modifying the Makefile directly. Adjust paths (CUDA, CXXCUDA), compiler flags (CXXFLAGS, LFLAGS), and dependencies (SRC, OBJET) as per your project's requirements.

## Windows

Install CUDA SDK 10.2 and open VC_CUDA102\Kangaroo.sln in Visual C++ 2019.\
You may need to reset your *Windows SDK version* in project properties.\
In Build->Configuration Manager, select the *Release* configuration.\
Build and enjoy.\
\
Note: The current release has been compiled with Visual studio 2019 and CUDA SDK 10.2, if you have a different release of the CUDA SDK, you may need to update CUDA SDK paths in Kangaroo.vcxproj using a text editor. The current nvcc option are set up to architecture starting at 3.0 capability, for older hardware, add the desired compute capabilities to the list in GPUEngine.cu properties, CUDA C/C++, Device, Code Generation.

Visual Sutido 2017 + Cuda 10 => Take project files in VC_CUDA10 (project files might be out of date)\
Visual Studio 2019 + Cuda10.2 => Take project files in VC_CUDA102\

## Linux

Install CUDA SDK.\
Depending on the CUDA SDK version and on your Linux distribution you may need to install an older g++ (just for the CUDA SDK).\
Edit the makefile and set up the good CUDA SDK path and appropriate compiler for nvcc. 

```
CUDA       = /usr/local/cuda-8.0
CXXCUDA    = /usr/bin/g++-4.8
```

You can enter a list of architecture (refer to nvcc documentation) if you have several GPU with different architecture. Compute capability 2.0 (Fermi) is deprecated for recent CUDA SDK.
Kangaroo need to be compiled and linked with a recent gcc (>=7). The current release has been compiled with gcc 7.3.0.\
Go to the Kangaroo directory. ccap is the desired compute capability.

```
$ g++ -v
gcc version 7.3.0 (Ubuntu 7.3.0-27ubuntu1~18.04)
$ make all (for build without CUDA support)
or
$ make gpu=1 ccap=20 all
```

Example with a 65bit key:
```
./kangaroo-256 65.txt
```

```
[+] Kangaroo v2.3 [256 range edition]
[+] Start:10000000000000000
[+] Stop :1FFFFFFFFFFFFFFFF
[+] Keys :1
[+] Number of CPU thread: 12
[+] Range width: 2^64
[+] Jump Avg distance: 2^31.99
[+] Number of kangaroos: 2^13.58
[+] Suggested DP: 15
[+] Expected operations: 2^33.10
[+] Expected RAM: 33.4MB
[+] DP size: 15 [0x0007fff]
[+] SolveKeyCPU Thread 06: 1024 kangaroos
[+] SolveKeyCPU Thread 01: 1024 kangaroos
[+] SolveKeyCPU Thread 04: 1024 kangaroos
[+] SolveKeyCPU Thread 02: 1024 kangaroos
[+] SolveKeyCPU Thread 00: 1024 kangaroos
[+] SolveKeyCPU Thread 09: 1024 kangaroos
[+] SolveKeyCPU Thread 10: 1024 kangaroos
[+] SolveKeyCPU Thread 11: 1024 kangaroos
[+] SolveKeyCPU Thread 03: 1024 kangaroos
[+] SolveKeyCPU Thread 08: 1024 kangaroos
[+] SolveKeyCPU Thread 07: 1024 kangaroos
[+] SolveKeyCPU Thread 05: 1024 kangaroos
[+] [55.52 MK/s][GPU 0.00 MK/s][Count 2^31.71][Dead 1][01:12 (Avg 02:45)][9.4/22.2MB] 
[+] Done: Total time 01:13 

```
All the public address and privatekeys will be saved in the file KEYFOUNDKEYFOUND.txt of your current directory.

# Articles

 - [1] Using Equivalence Classes to Accelerate Solvingthe Discrete Logarithm Problem in a Short Interval\
       https://www.iacr.org/archive/pkc2010/60560372/60560372.pdf
 - [2] Kangaroo Methods for Solving theInterval Discrete Logarithm Problem\
       https://arxiv.org/pdf/1501.07019.pdf
 - [3] Factoring and Discrete Logarithms using Pseudorandom Walks\
       https://www.math.auckland.ac.nz/~sgal018/crypto-book/ch14.pdf
 - [4] Kangaroos, Monopoly and Discrete Logarithms\
       https://web.northeastern.edu/seigen/11Magic/KruskalsCount/PollardKangarooMonopoly.pdf
