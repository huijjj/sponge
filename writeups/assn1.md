Assignment 1 Writeup
=============

My name: 정희종

My POVIS ID: hui0213

My student ID (numeric): 20190846

This assignment took me about 3 hours to do (including the time on studying, designing, and writing the code).

Program Structure and Design of the StreamReassembler:
StreamReassembler has 2 vector array. Both initialized with size of capacity.
One(unass) for storing actual unassembled bytes and other(bitmap) for storing indicators whether if each byte in unass is occupied or not.
public interface push_substring acts like below.
    1. check if given parameter index is within the bound of capacity
    2. insert given data(all or first few bytes) to unass if possible
    3. if given data is fully inserted check the given parameter eof and set eof_flag if needed
    4. check unass and see if there are assembled bytes
    5. (optional) if there are, write assembled bytes to _output and update internal states  

Implementation Challenges:
Considering all exceptions.

Remaining Bugs:
None.

- Optional: I had unexpected difficulty with: Considering all exceptions.

- Optional: I think you could make this assignment better by: Not sure if this can be called better but if students have to build their tests by there own while minimum interfaces for test provided, this assignment will be very difficult. Or at least difficult to get the full score.

- Optional: I was surprised by: How effective TDD is.

- Optional: I'm not sure about: [describe]
