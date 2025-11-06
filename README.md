# Operating Systems

A collection of low-level system programming projects in C, focusing on file systems, utilities, and network communication. This repository contains educational implementations of core operating system concepts.

## 📋 Table of Contents

- [Overview](#overview)
- [Repository Structure](#repository-structure)
- [Components](#components)
  - [Low-Level FileSystem](#low-level-filesystem)
  - [Low-Level Utility](#low-level-utility)
  - [Low-Level Communication](#low-level-communication)
- [Prerequisites](#prerequisites)
- [Building and Compilation](#building-and-compilation)
- [Usage Examples](#usage-examples)
- [Educational Context](#educational-context)
- [License](#license)

## Overview

This repository contains three main categories of low-level system programming projects:

1. **File System Implementation** - A custom FUSE-based filesystem (MyFS)
2. **Unix Utilities** - Custom implementations of common Unix commands
3. **Network Communication** - UDP/TCP network programming examples

All implementations are written in C and demonstrate fundamental operating system concepts including file I/O, memory management, process control, and network programming.

## Repository Structure

```
OperatingSystems/
├── Low-Level-FileSystem/     # FUSE filesystem implementation
│   ├── myfs.c                # Main FUSE filesystem driver
│   └── implementation.c      # Custom filesystem operations
├── Low-Level-Utility/        # Unix utility implementations
│   ├── head.c                # Display first N lines of a file
│   ├── tail.c                # Display last N lines of a file
│   └── findlocation.c        # Search for location data in files
└── Low-LevelCommunication/   # Network programming examples
    ├── send_udp.c            # UDP sender
    ├── receive_udp.c         # UDP receiver
    ├── reply_udp.c           # UDP echo server
    ├── send_receive_udp.c    # Bidirectional UDP communication
    ├── tunnel_udp_over_tcp_client.c  # UDP-over-TCP tunnel client
    └── tunnel_udp_over_tcp_server.c  # UDP-over-TCP tunnel server
```

## Components

### Low-Level FileSystem

**MyFS** is a custom filesystem implementation using FUSE (Filesystem in Userspace). It runs entirely in memory and can optionally persist data to a backup file.

#### Features:
- In-memory filesystem with optional backup file support
- Support for basic file operations: create, read, write, delete
- Directory operations: create, list, remove, rename
- File metadata: access times, modification times, permissions
- Thread-safe operations using mutexes
- Configurable filesystem size (default: 128MB, minimum: 2KB)

#### Files:
- `myfs.c` - FUSE driver and main entry point (DO NOT MODIFY)
- `implementation.c` - Custom filesystem implementation (student work goes here)

#### Building:
```bash
cd Low-Level-FileSystem
gcc -g -O0 -Wall myfs.c implementation.c `pkg-config fuse --cflags --libs` -o myfs
```

#### Usage:
```bash
# Mount the filesystem with a backup file
./myfs --backupfile=test.myfs ~/fuse-mnt/ -f

# In another terminal, unmount when done
fusermount -u ~/fuse-mnt
```

#### Debug Mode:
```bash
# Run with gdb for debugging
gdb --args ./myfs --backupfile=test.myfs ~/fuse-mnt/ -f
```

### Low-Level Utility

Custom implementations of common Unix utilities, demonstrating low-level file I/O operations.

#### 1. head.c
Displays the first N lines of a file or stdin.

**Features:**
- Configurable number of lines (default: 10)
- Support for stdin and file input
- Custom implementations of string and number parsing functions

**Compilation:**
```bash
cd Low-Level-Utility
gcc -Wall -o head head.c
```

**Usage:**
```bash
# Display first 10 lines of a file
./head filename.txt

# Display first 20 lines
./head -n 20 filename.txt

# Read from stdin
cat file.txt | ./head -n 15
```

#### 2. tail.c
Displays the last N lines of a file or stdin.

**Features:**
- Configurable number of lines (default: 10)
- Efficient circular buffer implementation
- Handles files without trailing newlines

**Compilation:**
```bash
cd Low-Level-Utility
gcc -Wall -o tail tail.c
```

**Usage:**
```bash
# Display last 10 lines of a file
./tail filename.txt

# Display last 20 lines
./tail -n 20 filename.txt

# Read from stdin
cat file.txt | ./tail -n 5
```

#### 3. findlocation.c
Searches for specific location data in memory-mapped files.

**Features:**
- Memory-mapped file I/O for efficient searching
- Linear and binary search implementations
- Fixed-width record searching (32-byte records)

**Compilation:**
```bash
cd Low-Level-Utility
gcc -Wall -o findlocation findlocation.c
```

**Usage:**
```bash
# Search for a location code in a file
./findlocation datafile.bin 123456
```

### Low-Level Communication

Network programming examples demonstrating UDP and TCP socket programming.

#### 1. send_udp.c
Simple UDP packet sender that reads from stdin and sends to a server.

**Compilation:**
```bash
cd Low-LevelCommunication
gcc -Wall -o send_udp send_udp.c
```

**Usage:**
```bash
./send_udp <server> <port>
# Then type messages to send
```

#### 2. receive_udp.c
UDP server that receives packets and displays them.

**Compilation:**
```bash
cd Low-LevelCommunication
gcc -Wall -o receive_udp receive_udp.c
```

**Usage:**
```bash
./receive_udp <port>
```

#### 3. reply_udp.c
UDP echo server that receives packets and sends them back.

**Compilation:**
```bash
cd Low-LevelCommunication
gcc -Wall -o reply_udp reply_udp.c
```

**Usage:**
```bash
./reply_udp <port>
```

#### 4. send_receive_udp.c
Interactive bidirectional UDP communication client.

**Compilation:**
```bash
cd Low-LevelCommunication
gcc -Wall -o send_receive_udp send_receive_udp.c
```

**Usage:**
```bash
./send_receive_udp <server> <port>
```

#### 5. UDP-over-TCP Tunneling

Implements tunneling of UDP packets over TCP connections, useful for traversing firewalls or networks that block UDP.

**Server Compilation:**
```bash
cd Low-LevelCommunication
gcc -Wall -o tunnel_server tunnel_udp_over_tcp_server.c
```

**Client Compilation:**
```bash
cd Low-LevelCommunication
gcc -Wall -o tunnel_client tunnel_udp_over_tcp_client.c
```

**Usage:**
```bash
# On the server side
./tunnel_server <tcp_port> <udp_server> <udp_port>

# On the client side
./tunnel_client <tcp_server> <tcp_port>
```

## Prerequisites

### Required Tools:
- **GCC** (GNU Compiler Collection)
- **Make** (optional, for build automation)
- **FUSE development libraries** (for filesystem component)
  ```bash
  # On Ubuntu/Debian:
  sudo apt-get install libfuse-dev pkg-config
  
  # On Fedora/RHEL:
  sudo dnf install fuse-devel
  
  # On macOS:
  brew install macfuse pkg-config
  ```

### System Requirements:
- Linux, macOS, or other Unix-like operating system
- POSIX-compliant system calls
- For FUSE: kernel support for FUSE (usually available by default)

## Building and Compilation

Each component can be compiled independently. Navigate to the specific directory and use the provided compilation commands.

### General Pattern:
```bash
# Basic compilation
gcc -Wall -o <output_name> <source_file>.c

# With optimization and debugging
gcc -g -O0 -Wall -o <output_name> <source_file>.c

# For FUSE filesystem
gcc -g -O0 -Wall myfs.c implementation.c `pkg-config fuse --cflags --libs` -o myfs
```

### Compile All Utilities:
```bash
# Compile all utilities at once
cd Low-Level-Utility
gcc -Wall -o head head.c
gcc -Wall -o tail tail.c
gcc -Wall -o findlocation findlocation.c

# Compile all network programs
cd ../Low-LevelCommunication
gcc -Wall -o send_udp send_udp.c
gcc -Wall -o receive_udp receive_udp.c
gcc -Wall -o reply_udp reply_udp.c
gcc -Wall -o send_receive_udp send_receive_udp.c
gcc -Wall -o tunnel_server tunnel_udp_over_tcp_server.c
gcc -Wall -o tunnel_client tunnel_udp_over_tcp_client.c

# Compile filesystem
cd ../Low-Level-FileSystem
gcc -g -O0 -Wall myfs.c implementation.c `pkg-config fuse --cflags --libs` -o myfs
```

## Usage Examples

### Example 1: Using head and tail together
```bash
# Show lines 10-20 of a file
head -n 20 file.txt | tail -n 10
```

### Example 2: UDP Communication
```bash
# Terminal 1: Start the receiver
cd Low-LevelCommunication
./receive_udp 8080

# Terminal 2: Send messages
./send_udp localhost 8080
# Type messages and press Enter
```

### Example 3: UDP Echo Server
```bash
# Terminal 1: Start echo server
./reply_udp 9000

# Terminal 2: Send and receive messages
./send_receive_udp localhost 9000
```

### Example 4: UDP-over-TCP Tunnel
```bash
# Terminal 1: Start UDP echo server
./reply_udp 5000

# Terminal 2: Start tunnel server
./tunnel_server 6000 localhost 5000

# Terminal 3: Use tunnel client
./tunnel_client localhost 6000
# Messages will be tunneled through TCP to the UDP server
```

### Example 5: Working with MyFS
```bash
# Create mount point
mkdir ~/fuse-mnt

# Mount filesystem
./myfs --backupfile=mydata.fs ~/fuse-mnt/ -f

# In another terminal, use the filesystem
cd ~/fuse-mnt
echo "Hello, World!" > test.txt
cat test.txt
mkdir mydir
ls -la

# Unmount when done
fusermount -u ~/fuse-mnt
```

## Educational Context

These projects are designed for learning fundamental operating systems concepts:

### Key Concepts Covered:

1. **File Systems**
   - FUSE (Filesystem in Userspace)
   - Memory management and mapping
   - File descriptors and operations
   - Thread synchronization

2. **Unix System Programming**
   - Low-level file I/O (`read`, `write`, `open`, `close`)
   - Memory-mapped files (`mmap`)
   - String manipulation without standard library
   - Custom implementations of standard functions

3. **Network Programming**
   - Socket programming (UDP and TCP)
   - Client-server architecture
   - Protocol design and implementation
   - Multiplexing with `select()`
   - Tunneling techniques

4. **Best Practices**
   - Error handling with `errno`
   - Resource cleanup and management
   - Buffer overflow prevention
   - Portable code writing

### Copyright and Attribution

**MyFS** is based on:
- Copyright 2018-21 by University of Alaska Anchorage, College of Engineering
- Copyright 2022-24 by University of Texas at El Paso, Department of Computer Science
- Contributor: Christoph Lauter, Luis Gomez, and Ivan Armenta
- Based on FUSE: Filesystem in Userspace, Copyright (C) 2001-2007 Miklos Szeredi

**Network and Utility Programs** are educational implementations for teaching purposes.

## License

This program can be distributed under the terms of the GNU GPL. See the file COPYING for details.

## Notes for Students

- For the MyFS project, **do not modify** `myfs.c` unless explicitly allowed by your instructor
- All custom filesystem code should go in `implementation.c`
- Use the provided utility functions and follow the existing code style
- Test thoroughly with various edge cases
- Use debugging tools like `gdb` for troubleshooting
- Always unmount FUSE filesystems properly to avoid data corruption

## Troubleshooting

### FUSE Issues:
```bash
# If mount fails, ensure FUSE is loaded
modprobe fuse

# Check if mount point is already in use
fusermount -u ~/fuse-mnt

# Run with verbose output
./myfs --backupfile=test.myfs ~/fuse-mnt/ -f -d
```

### Network Issues:
```bash
# Check if port is already in use
netstat -tuln | grep <port>

# Kill process using a port
fuser -k <port>/tcp
fuser -k <port>/udp
```

### Compilation Issues:
```bash
# Install missing FUSE libraries
sudo apt-get install libfuse-dev pkg-config

# Check pkg-config settings
pkg-config fuse --cflags --libs
```

---

**Happy coding and learning about operating systems!** 🚀