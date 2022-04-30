Assignment 5 Writeup
=============

My name: 정희종

My POVIS ID: hui0213

My student ID (numeric): 20190846

This assignment took me about 4 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the NetworkInterface:
NetworkInterface sends and receives ethernet frame. It sends datagram into lower level and retrieve datagram from lower level. To do so, NetworkInterface maintains cache of ip address and ethernet address mapping(translation in my code) for 30 sec. Whenever new frame arrives NetworkInterface learns new mapping. And If we have to send datagram to unknown ethernet address, NetworkInterface queues the datagram and send ARPMessage to request wanted ethernet address. To not flood the network with ARP message, request is sent to same host once in 5 sec.

Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
