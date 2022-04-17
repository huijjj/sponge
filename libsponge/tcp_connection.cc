#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {

    _time_since_last_segment_received = 0; // reset timer ticks

    const TCPHeader& header = seg.header(); // get header
    
    if(!_active) { // initiating hand shake
        if(header.syn) {
            _active = true;
        }
    }
    else {
        if(header.rst) { // rst flag set, kill connection
            _sender.stream_in().set_error();
            _receiver.stream_out().set_error();
            _active = false;

            return;
        }

        _receiver.segment_received(seg); // pass segment to receiver

        if(header.ack) { // if ACK is set
            _sender.ack_received(header.ackno, header.win); // notify sender
        }
        
        if(fin_sent) { // local endpoint has done sending its data first
            if(header.ackno == _sender.next_seqno()) { // fin acked
                fin_acked = true;   
                if(_linger_after_streams_finish == false) { // no need to wait
                    _active = false; // end connection
                }
            }
        }
        
        if(!fin_sent && _receiver.stream_out().input_ended()) { // remote endpoint has done sending its data first
            _linger_after_streams_finish = false;
        }
      
        
        if(_receiver.ackno().has_value()) { // SYN recieved
            _sender.fill_window();
        }

        if(seg.length_in_sequence_space() || (_receiver.ackno().has_value() && (seg.length_in_sequence_space() == 0) && (header.seqno == _receiver.ackno().value() - 1))) { // handling sending ack only and keep-alives
            if(_sender.segments_out().empty()) {
                _sender.send_empty_segment(); // make empty segment
            }
        }

        flush_segments(); // send all segments
    }
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    size_t ret = _sender.stream_in().write(data); // write data to sender's byte stream
    _sender.fill_window(); // make segments
    flush_segments(); // send segments

    return ret;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _time_since_last_segment_received += ms_since_last_tick;
    _sender.tick(ms_since_last_tick); 

    if(fin_acked && _receiver.stream_out().input_ended()) { // clean shutdown
        if(_linger_after_streams_finish) { // have to wait for certain amount of time to make sure
            if(_time_since_last_segment_received >= _cfg.rt_timeout * 10) { // wait for certain amount of time
                _active = false;
            } 
        }
        else { // don't need to wait
            _active = false;
        }
    }

    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) { // connection is hopeless
        // kill connection
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _active = false;
        
        _sender.send_empty_segment(); // generate empty segment

        TCPSegment s = _sender.segments_out().back();
        s.header().rst = true;

        _segments_out.push(s); // send segment with rst flag and kill connection

        return;
    }
    
    flush_segments(); // send all segments if exist
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input(); // end input
    _sender.fill_window(); // try to make segments if possible
    flush_segments(); // send segments if exist
}

void TCPConnection::connect() {
    _active = true; // activate connection
    _sender.fill_window(); // make SYN segment
    flush_segments(); // send segments if exist
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            // kill connection
            _sender.stream_in().set_error();
            _receiver.stream_out().set_error();
            _active = false;

            _sender.send_empty_segment(); // generate empty segment

            TCPSegment s = _sender.segments_out().back();
            s.header().rst = true;

            _segments_out.push(s); // send segment with rst flag and kill connection
        }   
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::flush_segments() { // flush all segments to outgoing queue
    while(!_sender.segments_out().empty()) {
        TCPSegment s = _sender.segments_out().front(); // pop segment
        _sender.segments_out().pop();

        s.header().win = _receiver.window_size(); // set win
        if(_receiver.ackno().has_value()) { // set ACK, ackno
            s.header().ack = true;
            s.header().ackno = _receiver.ackno().value();
        }

        if(s.header().fin) {
            fin_sent = true; // set fin_sent
        }

        _segments_out.push(s); // push segment to outgoing queue
    }
}