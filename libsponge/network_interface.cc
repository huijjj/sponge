#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <algorithm>
#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();

    if (translation.find(next_hop_ip) != translation.end()) {  // know ethernet addr
        EthernetFrame frame;
        frame.header().type = EthernetHeader::TYPE_IPv4;
        frame.header().src = _ethernet_address;
        frame.header().dst = translation[next_hop_ip].first;
        frame.payload() = dgram.serialize();

        _frames_out.push(frame);
    } else {                                               // don't know ethernet addr
        pending.push_back(make_pair(dgram, next_hop_ip));  // push datagram to pending queue

        if (resolving.find(next_hop_ip) == resolving.end() || resolving[next_hop_ip] >= 5000) {
            resolving[next_hop_ip] = 0;

            ARPMessage arp;
            arp.opcode = ARPMessage::OPCODE_REQUEST;
            arp.sender_ethernet_address = _ethernet_address;
            arp.sender_ip_address = _ip_address.ipv4_numeric();
            arp.target_ethernet_address = EthernetAddress{};
            arp.target_ip_address = next_hop_ip;

            EthernetFrame frame;
            frame.header().type = EthernetHeader::TYPE_ARP;
            frame.header().src = _ethernet_address;
            frame.header().dst = ETHERNET_BROADCAST;
            frame.payload() = arp.serialize();

            _frames_out.push(frame);
        }
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    const EthernetHeader &hdr = frame.header();

    if (hdr.dst != ETHERNET_BROADCAST && hdr.dst != _ethernet_address) {  // ignore
        return {};
    } else {
        if (hdr.type == EthernetHeader::TYPE_IPv4) {  // ipv4
            InternetDatagram ret;
            if (ret.parse(frame.payload()) != ParseResult::NoError) {  // error
                return {};
            } else {
                return ret;
            }
        } else {  // arp
            ARPMessage arp;
            if (arp.parse(frame.payload()) != ParseResult::NoError) {  // error
                return {};
            } else {
                uint32_t src_ip = arp.sender_ip_address;
                EthernetAddress src_eth = arp.sender_ethernet_address;

                resolving.erase(src_ip);
                translation[src_ip] = make_pair(src_eth, 0);  // cache mapping

                auto it = find_if(pending.begin(), pending.end(), [src_ip](pair<InternetDatagram, uint32_t> e) -> bool {
                    return e.second == src_ip;
                });

                while (it != pending.end()) {
                    EthernetFrame frm;
                    frm.header().type = EthernetHeader::TYPE_IPv4;
                    frm.header().src = _ethernet_address;
                    frm.header().dst = src_eth;
                    frm.payload() = (*it).first.serialize();

                    _frames_out.push(frm);

                    pending.erase(it);

                    it = find_if(pending.begin(), pending.end(), [src_ip](pair<InternetDatagram, uint32_t> e) -> bool {
                        return e.second == src_ip;
                    });
                }

                if (arp.opcode == ARPMessage::OPCODE_REQUEST && arp.target_ip_address == _ip_address.ipv4_numeric()) {
                    ARPMessage rep;
                    rep.opcode = ARPMessage::OPCODE_REPLY;
                    rep.sender_ethernet_address = _ethernet_address;
                    rep.sender_ip_address = _ip_address.ipv4_numeric();
                    rep.target_ethernet_address = src_eth;
                    rep.target_ip_address = src_ip;

                    EthernetFrame frm;
                    frm.header().type = EthernetHeader::TYPE_ARP;
                    frm.header().src = _ethernet_address;
                    frm.header().dst = src_eth;
                    frm.payload() = rep.serialize();

                    _frames_out.push(frm);
                }

                return {};
            }
        }
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    for (auto it = resolving.begin(); it != resolving.end(); it++) {
        (*it).second += ms_since_last_tick;
    }
    for (auto it = translation.begin(); it != translation.end(); it++) {
        (*it).second.second += ms_since_last_tick;
    }

    auto it = find_if(translation.begin(), translation.end(), [](auto e) -> bool { return e.second.second >= 30000; });

    while (it != translation.end()) {
        translation.erase(it);

        it = find_if(translation.begin(), translation.end(), [](auto e) -> bool { return e.second.second >= 30000; });
    }
}
