# iotupd - IoT Update Client and Server

Small client and server programs to demonstrate the idea of IoT updates over multicast.

The "server" is tiny.  It never needs to accept any connections, or listen on any ports.  It can sit behind a completely closed inbound firewall.  All it needs is to be able to send outbound UDP packets.

The server sends the file continuously on a loop, with size, offset  and checksum data embedded in each packet.  If no receivers are joined to the channel, all traffic is dropped by the first hop router (or switch with MLD), so no data is sent.  At most the data is sent once, regardless of how many nodes are listening.

The client can join the channel at any time, and will start writing the file immediately, even if halfway through the file.  Once it has received enough data it will begin checksumming and will stop writing when the checksums match.

Multiple servers can run at once, starting at different offsets in the file.  The client will receive from as many servers as are running.

Likewise, the server(s) can support as many clients as necessary with no additional load.


## Flow control

There is no flow control.  PKT_DELAY can be set to tell the server to slow down sending of packets, but there is no dynamic control at this time.  This is a simplistic demo.

There are various NACK solutions as described in PGM (RFC 3208) that we could employ here, at the cost of scalability.

We could instead run multiple streams at different rates, and let the client choose a channel with the appropriate receive rate.


## Reliability

If a packet is dropped, the client will need to get it on the next iteration.  FEC could be used to cope with minor packet loss.


## Deployment

This is just a simple demo, not intended for production.  It could, however, be adapted fairly easily to different scenarios.

You can run multiple servers at the same time, sending the same file.  A client joined via Any Source Multicast (ASM) can collect the packets from all servers at once.

Alternatively, with Single Source Multicast (SSM), we only have to turn on one setting on all intervening routers (`ipv6 multicast-routing`) and we have functioning multicast with no requirement for Rendezvous Points (RP) to be configured.  The IoT device could use plain ol' unicast DNS to look up which host(s) to do the SSM join to.  Some SRV records would do the trick.

So in the case of an IoT provider who controls the whole network, they could implement this immediately with no transitional tunnelling needed.

## Authors

Brett Sheffield

## License

This program is licensed under GPLv2 or (at your option) GPLv3.
