# MemoryManager (C++ Operating Systems Project)

## Overview
MemoryManager is a C++ simulation of an **operating system memory manager**.  
It allows the allocation and deallocation of memory, keeps track of free and occupied memory blocks, and demonstrates classic memory allocation algorithms:

- **Best-Fit Allocation**: Allocates the smallest free block that is large enough.
- **Worst-Fit Allocation**: Allocates the largest available block.

This project was developed for an **Operating Systems course** to explore memory management concepts such as fragmentation, allocation, and block tracking.

## Features
- Simulates memory as contiguous words in bytes.
- Tracks memory **holes** (free spaces) and **blocks** (allocated spaces).
- Supports **best-fit** and **worst-fit** allocation strategies.
- Memory dump to a file for analysis.
- Flexible memory word size and dynamic initialization.
- Modular class design for memory simulation.
