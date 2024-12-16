# Networking Mini Projects

## Project Overview

This repository contains two networking mini-projects implemented in C:

1. **XOXO (Multiplayer Tic-Tac-Toe)** - Located in `part-a`
2. **Fake It Till You Make It (TCP over UDP)** - Located in `part-b`

### Project A: XOXO - Multiplayer Tic-Tac-Toe

#### Features
- Multiplayer Tic-Tac-Toe game using network sockets
- Implemented with both TCP and UDP socket variants
- Server manages game state and player turns
- Supports game restart functionality

#### Networking Assumptions
- Supports exactly two clients
- Game flow controlled by server
- Playable on local or remote machines

#### How to Run
1. Compile the server and client executables
2. Start the server: `./server [optional_ip_address]`
3. Connect two clients to the server
4. Play Tic-Tac-Toe!

### Project B: Fake It Till You Make It - TCP Simulation over UDP

#### Features
- Implements TCP-like functionality using UDP
- Data sequencing with chunk-based transmission
- Automatic retransmission of lost packets
- Chunk-based data transfer

#### Implementation Details
- **Chunk Size**: 5 bytes
- **Timeout**: 0.1 seconds
- **Buffer Size**: 1024 bytes
- Simulates TCP functionalities:
  - Data sequencing
  - Packet retransmission

#### Networking Assumptions
- Simple one-time message transfer
- No full TCP connection management (no 4-way handshake)
- Supports local and remote machine communication

#### How to Run
1. Compile the server and client executables
2. Start the server: `./server [optional_ip_address]`
3. Run the client to send messages

### Prerequisites
- C compiler (gcc recommended)
- Basic understanding of socket programming
- Unix-like operating system (Linux/macOS preferred)

### Compilation
```bash
# For XOXO (TCP version)
cd PART\ A/
gcc server_tcp.c -o server_tcp
gcc client_tcp.c -o client_tcp

# For XOXO (UDP version)
gcc server_udp.c -o server_udp
gcc client_udp.c -o client_udp

# For Fake It Till You Make It
cd PART\ B/
gcc server.c -o server
gcc client.c -o client
```

### Limitations
- Limited error handling
- Basic implementation of networking concepts
- Meant for educational purposes

### Contributing
Feel free to fork and improve the projects. Pull requests are welcome!

### Author
Kushagra Trivedi
