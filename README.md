# Networking Assumptions
## XOXO
- The code can handle at most two clients. 
- The game will only exit/restart until a response is heard from both clients. Even if one declines, the other still has to respond and the socket will not be closed properly. This is because `recv` is a blocking call.
- If you want to handle the game on two different machines you need to type in this format ```the compiled file executable and IP address of the server``` example ```./server 192.168.1.233```

## Fake it till you make it
- The code implements TCP using UDP by dividing the data into smaller chunks. Each chunk is assigned a number which is sent along with the transmission (use structs). The sender should also communicate the total number of chunks being sent. After the receiver has data from all the chunks, it should aggregate them according to their sequence number and display the text.
- CHUNK_SIZE - 5 bytes
- TIMEOUT - 0.1 seconds
- BUFFER_SIZE - 1024 bytes (max input length)
- The code only runs once (simple while loop can be implemented to continously send messages)
- No code is implemented for 4-way handshake during establishing connection and ending connection using FYN/RST flags that happens in real TCP sockets for connection management. Only the mentioned functionalities were implemented:
    - Data Sequencing
    - Retransmissions
- If you want to handle the communication on two different machines you need to type in this format ```the compiled file executable and IP address of the server``` example ```./server 192.168.1.233```

