#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : bitmap(capacity, false), unass(capacity, ' '), _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (data.size()) {                // ignore empty string
        if (!active) {                // no assembled data yet
            if (index < _capacity) {  // data is within capacity, handling data is possible
                size_t inserted = 0;
                size_t end = index + data.size();
                if (end > _capacity) {
                    end = _capacity;
                }
                for (size_t i = index; i < end; i++) {
                    unass[i] = data[i - index];
                    bitmap[i] = true;
                    inserted++;
                }
                if (inserted == data.size()) {  // if data is fully handled
                    if (eof) {
                        _eof = true;
                    }
                }
                if (last_index < index + inserted - 1) {  // update last index
                    last_index = index + inserted - 1;
                }
            }
        } else {
            if (cur >= index) {                       // data overlaps with already assembled one
                if (index + data.size() - 1 > cur) {  // if data is already assembled, ignore
                    size_t _index = cur + 1;
                    const string _data = data.substr(cur - index + 1);

                    size_t inserted = 0;
                    size_t bytes_to_insert = _data.size();
                    if (_data.size() > _output.remaining_capacity()) {  // data bigger than remaining capacity
                        bytes_to_insert = _output.remaining_capacity();
                    }
                    for (size_t i = 0; i < bytes_to_insert; i++) {
                        unass[i] = _data[i];
                        bitmap[i] = true;
                        inserted++;
                    }
                    if (inserted == _data.size()) {  // if data is fully handled
                        if (eof) {
                            _eof = true;
                        }
                    }
                    if (last_index < _index + inserted - 1) {
                        last_index = _index + inserted - 1;
                    }
                }
            } else if (index - cur - 1 < _output.remaining_capacity()) {  // index is within capacity
                size_t inserted = 0;
                size_t bytes_to_insert = data.size();
                if (bytes_to_insert + index - cur - 1 > _output.remaining_capacity()) {
                    bytes_to_insert = _output.remaining_capacity() - (index - cur - 1);
                }
                for (size_t i = 0; i < bytes_to_insert; i++) {
                    unass[index - cur - 1 + i] = data[i];
                    bitmap[index - cur - 1 + i] = true;
                    inserted++;
                }
                if (inserted == data.size()) {  // if data is fully handled
                    if (eof) {
                        _eof = true;
                    }
                }
                if (last_index < index + inserted - 1) {
                    last_index = index + inserted - 1;
                }
            }
        }

        if (bitmap[0]) {  // there exists reassembled bytes
            size_t reassembled_bytes = 0;
            string reassembled = "";
            while (reassembled_bytes < _capacity && bitmap[reassembled_bytes]) {  // count reassembled bytes
                reassembled.push_back(unass[reassembled_bytes]);
                reassembled_bytes++;
            }
            _output.write(reassembled);  // write data to output

            // update current index information
            if (!active) {
                cur += (reassembled_bytes - 1);
            } else {
                cur += reassembled_bytes;
            }

            if (unassembled_bytes() == 0 && _eof) {  // all bytes in _output
                _output.end_input();
            }

            // clear internal data structure
            unass.erase(unass.begin(), unass.begin() + reassembled_bytes);
            bitmap.erase(bitmap.begin(), bitmap.begin() + reassembled_bytes);
            unass.resize(_capacity, ' ');
            bitmap.resize(_capacity, false);
            active = true;
        }
    } else {        // empty data
        if (eof) {  // if empty data with eof arrives
            if (index == 0 && last_index == 0) {
                _eof = true;
                _output.end_input();
            } else if (index - last_index == 1) {
                _eof = true;
                if (unassembled_bytes() == 0) {
                    _output.end_input();
                }
            }
        }
    }
}

size_t StreamReassembler::unassembled_bytes() const { return last_index - cur; }

bool StreamReassembler::empty() const { return last_index == cur; }
