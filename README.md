# CSSE2310 Semester Two 2021 Assignments

My C assignments written for the University of Queensland course CSSE2310 (Computer Systems and Programming) and some reflections on how I could have better
completed them in hindsight. 

## Assignment 1

### Description

The objective was to write a game of Battleships playable between the player and a CPU, with all relevant information such as map parameters, ship locations and CPU moves read in from files. 

Received 41.03/42 (97.70%) for functionality.

### How I would improve this assignment

* Improve the data structures

Many of the data structures used in this assignment could have been improved. For example, usage of only one struct to store 
the entirety of the game state was a poor decision: this should have been split into multiple structs so that only the relevant information would need to be passed to individual functions. 

Additionally, using structs to store information such as ship information would have reduced the complexity of the program; instead of allocating and deallocating 3D arrays representing ships, using structs would have been much easier to allocate and work with. 

* Reduce code duplication

Some functions were duplicated when their functionality could have been encapsulated into one function. For example, a single function could have handled adding a ship to the player's grid, instead of writing individual functions for each cardinal direction.

## Assignment 2

### Description

The objective was to build upon the previous assignment and write three programs (one hub and two players) to participate in a Battleships tournament using pipes and forks. 

Received 40.83/42 (97.21%) for functionality.

### How I would improve this assignment

* Reduce nesting of structs

Some structs such as agentGuesses were nested unnecessarily; this made it more cumbersome to write the program (some parts of the program required very long lines of nested truct accesses) and introduced some minor errors. 

* Handle errors with enums instead of integers

It would have been a better idea to return defined enums to signal function success or failure rather than use integers; using integers made it harder to remember what state each integer represented. 

## Assignment 3

### Description

The objective was to build a simple server and client to allow for arbitary numbers of clients to play Rock Paper Scissors using the server as a matchmaker. This was achieved using multithreading and socket programming. 

Received 42/42 (100%) for functionality.

### How I would improve this assignment

* Memory management
Some of the memory in these programs was not manually freed; this did not affect the functionality of the program as the program was run on an OS capable of reclaiming memory, but it would have been much better practice to free all memory that was allocated.
