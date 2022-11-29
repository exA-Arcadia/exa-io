JVS-Dash Protocol
=================

# Command 0F2h: Comm. Method Change (COMMCHG)

| Command Name            | Request Data        |              | Response Data |
|-------------------------|---------------------|--------------|---------------|
| **Comm. Method Change** | **F2, method code** |              | -             |
|                         | **F2**              | Command Code | -             |
|                         | **(01)**            | Method Code  | -             |


Changes the master and slave communication methods, such as to increase communication speed. This is a broadcast packet, so it is necessary to check that all I/O units are capable of the command sent. Whether this is supported can be determined by reading each I/O unit's JVS Revision to check for Dash support, then by querying the method support as specified below.

After setting a new communications speed, wait >5mS before sending new packets.

Because this is a broadcast packet without a response, no other packets should be bundled at the same time. After the broadcast, each node should be queried to test and verify the new speed.

# Command 0D0h: Comm. Support Query (COMMSUP)

| Command Name            | Request Data        |              | Response Data |         |
|-------------------------|---------------------|--------------|---------------|---------|
| **Comm. Support Query** | **D0, method code** |              |               |         |
| byte0                   | **D0**              | Command Code | (01)          | Report  |
| byte1                   | -                   | Method Code  | (01)          | Bitmask |
| byteN                   |                     |              |               | Bitmask |


Queries the values allowed to be sent to COMMCHG. The number of bytes required to send all data bytes is sent, followed by the data. Bit 0 in the bitmask should always be set as it is the default JVS speed. Any bits not sent due to the response length should be assumed to be 0. If bit7 is 1, there is another byte of data.

**EXAMPLE**: If the support bitmask returned is 43, then COMMCHG may be sent with either a parameter of 00, 01, or 06.

| Support Bitmask Mapping |          |          |          |          |          |          |          |          |
|-------------------------|----------|----------|----------|----------|----------|----------|----------|----------|
|                         | **bit7** | **bit6** | **bit5** | **bit4** | **bit3** | **bit2** | **bit1** | **bit0** |
| byte0                   | -        | -        | -        | -        | -        | 3Mbps    | 1Mbps    | 115200   |
| byte1                   | -        | -        | -        | -        | -        | -        | -        | -        |
