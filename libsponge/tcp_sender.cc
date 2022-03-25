#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _RTO{retx_timeout} {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    if(_next_seqno == 0) { // SYN is not acknowledged yet
        TCPSegment s;
        s.header().seqno = _isn;
        s.header().syn = true;
        _on_flight.push(s);
        _bytes_in_flight += 1;
        _segments_out.push(s);
        _next_seqno++;
        if(!_timeout.has_value()) {
            _timeout.emplace(_RTO);
        }
        return;
    }

    // SYN is acknowledged
    if(_stream.input_ended()) { // we have to consider FIN
        // size_t bytes_to_send = _window_size ? _window_size : 1;
        size_t bytes_to_send = _window_size ? _ackno.value() + _window_size - _next_seqno : 1;
        if(bytes_to_send >= _stream.buffer_size() + 1) { // receiver window can handle all the data
            bytes_to_send = _stream.buffer_size();
            while(bytes_to_send) {
                TCPSegment s;
                
                // fill up seqno
                s.header().seqno = wrap(_stream.bytes_read() + 1, _isn);
                
                // fill up payload and FIN flag
                if(bytes_to_send > TCPConfig::MAX_PAYLOAD_SIZE) {
                    s.payload() = static_cast<Buffer>(_stream.read(TCPConfig::MAX_PAYLOAD_SIZE));
                    bytes_to_send -= TCPConfig::MAX_PAYLOAD_SIZE;
                }
                else {
                    s.header().fin = true;
                    s.payload() = static_cast<Buffer>(_stream.read(bytes_to_send));
                    bytes_to_send = 0;
                }

                // push segment into queue
                _bytes_in_flight += s.length_in_sequence_space();
                if(!_timeout.has_value()) {
                    _timeout.emplace(_RTO);
                }
                _on_flight.push(s);
                _segments_out.push(s);
                _next_seqno += s.length_in_sequence_space();
            }
        }
        else { // receiver window can't handle all the data
            while(bytes_to_send) {
                TCPSegment s;

                // fill up seqno
                s.header().seqno = wrap(_stream.bytes_read() + 1, _isn);

                // fill up payload
                if(bytes_to_send > TCPConfig::MAX_PAYLOAD_SIZE) {
                    s.payload() = static_cast<Buffer>(_stream.read(TCPConfig::MAX_PAYLOAD_SIZE));
                    bytes_to_send -= TCPConfig::MAX_PAYLOAD_SIZE;
                }
                else {
                    s.payload() = static_cast<Buffer>(_stream.read(bytes_to_send));
                    bytes_to_send = 0;
                }

                // push segment into queue
                _bytes_in_flight += s.length_in_sequence_space();
                if(!_timeout.has_value()) {
                    _timeout.emplace(_RTO);
                }
                _on_flight.push(s);
                _segments_out.push(s);
                _next_seqno += s.length_in_sequence_space();
            }
        }
    }
    else { // we don't have to consider FIN yet
        if(_stream.buffer_empty()) {
            return;
        }
        // uint16_t bytes_to_send = _window_size ? _window_size : 1;
        size_t bytes_to_send = _window_size ? _ackno.value() + _window_size - _next_seqno : 1;
        while(bytes_to_send) {
            TCPSegment s;
            
            // fill up seqno
            s.header().seqno = wrap(_stream.bytes_read() + 1, _isn);

            // fill up the payload
            if(bytes_to_send > TCPConfig::MAX_PAYLOAD_SIZE) {
                if(_stream.buffer_size() > TCPConfig::MAX_PAYLOAD_SIZE) {
                    s.payload() = static_cast<Buffer>(_stream.read(TCPConfig::MAX_PAYLOAD_SIZE));
                    bytes_to_send -= TCPConfig::MAX_PAYLOAD_SIZE;
                }
                else {
                    s.payload() = static_cast<Buffer>(_stream.read(_stream.buffer_size()));
                    bytes_to_send = 0;
                }
            }
            else {
                if(bytes_to_send > _stream.buffer_size()) {
                    bytes_to_send = _stream.buffer_size();
                }
                s.payload() = static_cast<Buffer>(_stream.read(bytes_to_send));
                bytes_to_send = 0;
            }

            // push segment into queue
            _bytes_in_flight += s.length_in_sequence_space();
            if(!_timeout.has_value()) {
                _timeout.emplace(_RTO);
            }
            _on_flight.push(s);
            _segments_out.push(s);
            _next_seqno += s.length_in_sequence_space();
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    uint64_t abs_ack = unwrap(ackno, _isn, _next_seqno);
    if(_ackno.has_value()) {
        if(abs_ack > _ackno.value()) {
            _bytes_in_flight -= (abs_ack - _ackno.value());
            _ackno.emplace(abs_ack);
        }
    }
    else {
        _bytes_in_flight -= abs_ack;
        _ackno.emplace(abs_ack);
    }

    // remove acknowledged segments from queue
    while(!_on_flight.empty()) {
        const TCPSegment& s = _on_flight.front();
        if(unwrap(s.header().seqno, _isn, _next_seqno) + s.length_in_sequence_space() <= abs_ack) { // this segment is fully acknowledged
            _on_flight.pop();
            _RTO = _initial_retransmission_timeout; // reset RTO
            _retransmission_count = 0; // reset retransmisison count
            if(_on_flight.empty()) {
                _timeout.reset(); // clear timer
                break;
            }
            else {
                _timeout.emplace(_RTO); // reset timer
            }
        } 
        else {
            break;
        }
    }

    _window_size = window_size;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if(ms_since_last_tick < _timeout.value()) {
        _timeout.emplace(_timeout.value() - ms_since_last_tick); // update timer value
    }
    else { // timer expired
        _segments_out.push(_on_flight.front()); // retransmit the earliest segment
        if(_window_size) { // if window size is non zero
            _retransmission_count++; // increase retransmission count
            _RTO *= 2; // exponential backoff
        }
        _timeout.emplace(_RTO); // reset timer
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _retransmission_count; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}
