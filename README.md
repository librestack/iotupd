# iotupd - IOT Update Client and Server

send files continuously via multicast


Small client and server programs to demonstrate the idea of IOT multicast
updates.

The "server" is tiny.  It never needs to accept any connections, or listen on
any ports.  It can sit behind a completely closed inbound firewall.  All it
needs is to be able to send outbound UDP packets.

The server sends the file continuously on a loop, with size, offset  and
checksum data embedded in each packet.

The client can join the channel at any time, and will start writing the file
immediately, even if halfway through the file.  Once it has received enough data
it will begin checksumming and will stp writing when the checksums match.

Multiple servers can run at once, starting at different offsets in the file.
The client will receive from as many servers as are running.

Likewise, the server(s) can support as many clients as necessary with no
additional load.


## Flow control

There is no flow control.  PKT_DELAY can be set to tell the server to slow down
sending of packets, but there is no dynamic control at this time.  This is a
simplistic demo.

There are various NACK solutions as described in PGM that we could employ here,
at the cost of scalability.

We could also run multiple streams at different rates, and let the client choose
an appropriate receive rate.


## Reliability

If a packet is dropped, the client will need to get it on the next iteration.
FEC could be used to cope with minor packet loss.
