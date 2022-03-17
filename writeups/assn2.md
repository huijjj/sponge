Assignment 2 Writeup
=============

My name: 정희종

My POVIS ID: hui0213

My student ID (numeric): 20190846

This assignment took me about 2 hours to do (including the time on studying, designing, and writing the code).

If you used any part of best-submission codes, specify all the best-submission numbers that you used (e.g., 1, 2): []

- **Caution**: If you have no idea about above best-submission item, please refer the Assignment PDF for detailed description.

Program Structure and Design of the TCPReceiver and wrap/unwrap routines:
TCPReceiver::segmet_receiced decodes given segment and passes the data to stream reassembler with index converted from seqno in the header of the segment. To convert seqno to index we use unwrap to convert seqno to abs_seqno and convert to index by subtracting 1(1 byte for SYN).

TCPReceiver::ackno and TCPReceiver::window_size just simply returns data stored in buffer(bytes written, remaining capacity) with little modification.

wrap converts absolute seqno to seqno by adding ISN and doing the modulo operation. For the ease of implementation I used bitmask of lower 32 bits instead of actual modulo operator.

unwrap convers seqno to absolute seqno. This is done by finding the closest number to the checkpoint starting from given n whlie adding or subtracting 2^32.

Implementation Challenges:
Converting into various types were confusing(seqno, absolute seqno, index).

Remaining Bugs:
none.

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this assignment better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
