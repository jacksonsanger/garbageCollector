# Heap Memory Defragmentation - Garbage Collector

## Overview  
This project implements a custom memory management system that mimics aspects of modern garbage collection and memory compaction techniques. It introduces features like generational memory management, defragmentation, and a managed pointer system to simulate behavior seen in garbage-collected languages such as Java.

## Features  

### Generational Garbage Collection  
- **Young Generation Compaction**:
  - Uses a double-heap system to defragment memory by moving live objects between heaps, eliminating fragmentation without additional overhead.  
- **Old Generation Compaction**:
  - Implements in-place memory compaction in the old generation heap to consolidate memory and reduce fragmentation over time.  

### Managed Pointers  
- Indirect memory management system allows pointers to remain valid even if memory is relocated during garbage collection.
- Safeguards against dangling pointers by nullifying entries in the managed pointer list upon deallocation.

### Memory Allocation Strategies  
- Supports **First-Fit** and **Best-Fit** allocation strategies for dynamic memory allocation.
- Includes survival tracking for memory blocks, promoting frequently accessed blocks from the young generation to the old generation.

### Debugging Tools  
- **Enhanced Memory Dump**:
  - Displays detailed information about all memory blocks, including addresses, sizes, and usage status.
  - Provides a visual representation of memory usage for easier debugging.
- **Managed Pointer Visualization**:
  - Lists all managed pointers and their corresponding heap locations.


## Technical Details  
- **Language**: C  
- **Core Components**:
  - Double heaps for generational garbage collection.
  - Free list management for dynamic memory allocation.
  - Survival counters to determine memory block promotions.
  - In-place compaction for efficient old generation management.  
