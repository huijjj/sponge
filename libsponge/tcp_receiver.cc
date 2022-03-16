#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader hdr = seg.header();

    if (hdr.syn) {
        _isn.emplace(hdr.seqno);
    }

    if (_isn.has_value()) {
        size_t abs_ack = _reassembler.stream_out().bytes_written() + 1;
        size_t abs_seq = unwrap(hdr.seqno, _isn.value(), abs_ack);
        if (hdr.syn) {
            _reassembler.push_substring(seg.payload().copy(), abs_seq, hdr.fin);
        } else {
            _reassembler.push_substring(seg.payload().copy(), abs_seq - 1, hdr.fin);
        }
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_isn.has_value()) {
        size_t abs_ack = _reassembler.stream_out().bytes_written();  // count of bytes written to buffer
        abs_ack++;                                                   // 1 byte for SYN
        if (_reassembler.stream_out().input_ended()) {
            abs_ack++;  // 1byte for FIN
        }
        return wrap(abs_ack, _isn.value());
    } else {
        return {};
    }
}

size_t TCPReceiver::window_size() const { return _reassembler.stream_out().remaining_capacity(); }
