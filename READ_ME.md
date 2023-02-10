# Advanced and Robot Programming - Assignment 2
## Authors: Rocca Giovanni, Moriconi Michele

<br>

### Introduction
An overview of this program function.<br>
[Go to Introduction](#introduction)

### Installation and Execution
How install and run this program in Linux.<br>
[Go to Installation and Execution](#installation_and_execution)

### Legend of Buttons
Legend of the buttons clickable.<br>
[Go to Legend of Buttons](#legend_of_buttons)

### How it works
A rapid description of how the program works.<br>
[Go to How it works](#how_it_works)


<a name="introduction"></a>
### Introduction

The goal of this assignment is to design, develop, test and deploy the code for an interactive simulator of a (simplified) typical
vision system, able to track an object in a 2-D plane.<br>
This assignment requires the use of a shared memory in which two processes operate
simultaneously, as happens in reality in similar applications.<br>
In our case we don't have a camera, so we will simulate the creation of the moving image using an
ncurses window. Using arrow keys, we will move a spot in a window to simulate the perception of
the camera. The spot that we will see by moving will produce the creation of a realistic RGB image in the simulated, shared, video memory while in a second ncurses window, also 80 x 30, the position trace of the center of the
image will be shown.<br>
There will be an additional function, useful for debugging. By pressing a
key, or by operating the mouse on a button, a snapshot of the image memory will be saved on a
.bmp file.<br>

<a name="legend_of_buttons"></a>
### Legend of Buttons

Keyboard Buttons:

* Key Down -> moves the cursor down.
* Key Up -> moves the cursor up.
* Key Left -> moves the cursor to the left.
* Key Right -> moves the cursor to the right.

Window Button:

* P (blue button): print button, save a snapshot of the current bmp image.

<a name="installation_and_execution"></a>
### Installation and Execution

Two GitHub libraries will be used:
* ncurses, which was already used in the first assignment
* libbitmap.

## *libbitmap* installation and usage
To work with the bitmap library, you need to follow these steps:
1. Download the source code from [this GitHub repo](https://github.com/draekko/libbitmap.git) in your file system.
2. Navigate to the root directory of the downloaded repo and run the configuration through command ```./configure```. Configuration might take a while.  While running, it prints some messages telling which features it is checking for.
3. Type ```make``` to compile the package.
4. Run ```make install``` to install the programs and any data files and documentation.
5. Upon completing the installation, check that the files have been properly installed by navigating to ```/usr/local/lib```, where you should find the ```libbmp.so``` shared library ready for use.
6. In order to properly compile programs which use the *libbitmap* library, you first need to notify the **linker** about the location of the shared library. To do that, you can simply add the following line at the end of your ```.bashrc``` file:      
```export LD_LIBRARY_PATH="/usr/local/lib:$LD_LIBRARY_PATH"```

1.Open the terminal

2.Download the repository:

<pre><code>git clone https://github.com/MickyMori/ARP_Assignment_2</code></pre>

This repository contains:
- This 'README.md' file.
- The `src` folder, which contains the source code for the ProcessA, ProcessB and Master processes.
- The `include` folder contains all the data structures and methods used. You can ignore the content of this folder, as it already provides you with all the necessary functionalities.
- The `compiler.sh` and `run.sh` files.

3.Compile the source files:

<pre><code>bash compiler.sh</code></pre>

Once this command has been run, `logFiles`, `out` and `bin` folders will be created.

4.Execute the program:

<pre><code>bash run.sh</code></pre>

If you want to read a specific log file of a process:

Go in the log files directory:
<pre><code>cd logFiles</code></pre>

Process A log file:
<pre><code>cat processA.log</code></pre>

Process B log file:
<pre><code>cat processB.log</code></pre>

Master log file:
<pre><code>cat master.log</code></pre>

If you want to see a snapshot created by pressing the print button you have to go to `out` directory.


<a name="how_it_works"></a>
### How it works

There are 3 processes:

* <b>Process A</b>: draws a green circle in the bitmap, which center corresponds to the same location where the cursor is every time the user moves it using the keyboard buttons [See the Legend of clickable Buttons](#legend_of_buttons).
This process passes all the bmp map to the Process B using the shared memory.
One of the main problems of the use of the shared memory is that a situation arises in which are done the operations of reading and writing from memory at the same time, and so, we use combined semaphores to avoid this type of situation.     

* <b>Process B</b>: extracts from the bmp passed by the Process A, the location of the circle drawn in it and calculate the center of it and after this process re-draw the same circle on another bitmap. This process also keeps track of the cursor that is moved by the user through the Process A.

* <b>Master</b>: his main function is to start all the processes.
