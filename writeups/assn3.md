Assignment 3 Writeup
=============

My name: 정희종

My POVIS ID: hui0213

My student ID (numeric): 20190846

This assignment took me about 6 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the TCPSender:
I added a queue to my TCPSender to keep the information of bytes in flight. And other member variables to keep track of connection state. Using this member variables TCPSender::fill_window identifies receiver's state and do the appropriate action. And TCPSender::ack_received updates this member variables to proper status according to the information advertised by receiver. Last but not least, TCPSender::tick handles timer timeout.

Implementation Challenges:
Considering all edge cases. I couldn't have done it without given test cases.

Remaining Bugs:
none.

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
