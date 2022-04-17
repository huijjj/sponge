Assignment 4 Writeup
=============

My name: 정희종

My POVIS ID: hui0213

My student ID (numeric): 20190846

This assignment took me about 7 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Your benchmark results (without reordering, with reordering): [0.00, 0.00]

Program Structure and Design of the TCPConnection:
TCPConnection wires TCPSender and TCPReceiver implemented before. Manages some internal state for starting and closing the connection. But basically almost all functionalities are implemented in the sender and receiver. TCPConnection just simply passes received segments to receiver and notify sender and send the segments that sender wants to send. Also, TCPConnection also provides outer interface for reading and writing through TCP.

My TCP implementation achieved 0.15 Gbit/s without reordering and 0.92 Gbit/s with redordering.

Implementation Challenges:
Implementing shutdown logic.

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
