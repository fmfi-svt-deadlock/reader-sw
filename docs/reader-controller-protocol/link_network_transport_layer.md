Link, Network and Transport layers
==================================

Goal of this part of the protocol stack is to provide a reliable datagram communication between exactly two devices connected using a point-to-point link.

For this purpose we will use a protocol heavily inspired by [HDLC (High-Level Data Link Control)](https://en.wikipedia.org/wiki/High-Level_Data_Link_Control), but simplified for our use-case.

Although this protocol is similar to HDLC it may be different in subtle details, so treat this specification as a single source of truth.

Framing
-------

Frames of our protocol are transmitted using a byte-oriented link. The frame format is as follows (the tranmission order is from left to right):

```eval_rst

====== ========= ========= ====== ========= ======
 Flag   Address   Control   Info   FCS       Flag
====== ========= ========= ====== ========= ======
 0x7E   8 bits    8 bits    `*`    16 bits   0x7E
====== ========= ========= ====== ========= ======

```

Where

  - Flag = flag sequence, fixed to 0x7E
  - Address = data station address field
  - Control = control field
  - Info = Information field (may be of 0 length for frames containing only control sequence)
  - FCS = Frame Checking Sequence

All fields are transmitted low-order bit first.

### Frame elements

#### Flag sequence

This flag is used for frame synchronization. The receiving station shall continuously hunt for this sequence.

#### Address field

This field is present only for compatibility of frames with HDLC frames. Our protocol uses fixed All-station address (0xFF).

#### Control field

The control field format is similar to the "modulo 8" control field format defined in ISO/IEC 13239:2002(E), section 5.3.1:

```eval_rst

+-----------------------------+-----------------------------------------------+
| Control field format for    | Control field bit                             |
|                             +-----+-----+-----+-----+-----+-----+-----+-----+
|                             |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |
+=============================+=====+=====+=====+=====+=====+=====+=====+=====+
| Information xfer (I format) |  0  | N(S)            | P/F | N(R)            |
+-----------------------------+-----+-----+-----------+-----+-----------------+
| Supervisory cmds (S format) |  1  |  0  |  SUP      | P/F | N(R)            |
+-----------------------------+-----+-----+-----------+-----+-----------------+
| Unnumbered cmds (U format)  |  1  |  1  |  M_1      | P/F | M_2             |
+-----------------------------+-----+-----+-----------+-----+-----------------+

```

where

  - N(S) is the send sequence number
  - N(R) is the receive sequence number
  - P/F is the poll/final bit
  - SUP are two Supervisory bits
  - M_1 and M_2 are 2 (or 3, respectively) bits of Modifier bits
      + Modifier bits will be referred to as M. Value of M is a bit concatenation of M_1 and M_2.

#### Information field

Information field may carry any sequence of octets. Maximum length of the information field shall be 249 bytes.

#### Frame checking sequence

The frame checking sequence is as described in ISO/IEC 13239:2002(E), section 4.2.5.2 (16-bit frame checking sequence).

### Frame transparency

This protocol uses frame transparency similar to the one described in ISO/IEC 13239:2002(E), section 4.3.2.2 (Control-octet transparency).

#### Frame transmission

After the FCS calculation, all bytes between the openening and closing flag sequence (including address, control and FCS) are examined. If during examination the transmitter finds either the control escape byte (0x7D) or flag sequence (0x7E) it replace it with two bytes: the control escape byte (0x7D) and the offending byte XOR 0x20 (offending byte with the 6th bit complemented).

#### Frame reception

The receiver shall, when receiving a frame, after reception of the start control flag and before the reception of the end control flag examine each byte and:

  - If it finds the control escape byte (0x7D) it shall discard it
  - XOR the byte immediatelly following the discarded control escape byte with 0x20 (invert its 6th bit).

### Frame types

Communication between interconnected devices occurs using the following frames

#### DATA frame

DATA frame has I format Control Field with the P/F bit set and contains the Info section. It is used for data exchange between stations.

#### DATA_ACK frame

DATA_ACK frame has S format Control Field, where SUP bits are 0b00 (HDLC Receive Ready command) and P/F bit is set. It is used to acknowledge received DATA frames.

#### NACK frame

NACK frame has is used to reject DATA frames. It has S format Control Field, where SUP bits are 0b01 (HDLC Reject command) with P/F bit set.

#### CONN frame

CONN frame is used to initiate a connection. It has U format Control Field, where M is 0x7C and the P/F bit is set (HDLC SABM command).

#### CONN_ACK frame

CONN_ACK is used to acknowledge either the CMD_CONNECT or CMD_DISCONNECT. It has U format Control Field, where M is 0x66 (HDLC UA Response) and the P/F bit is set.

#### DISCONNECTED frame

DISCONNECTED frame is used as a response when the station is in disconnected mode. It has U format Control Field, where M is 0x78 and the P/F bit is set.

Procedures of operation
-----------------------

Procedures of operation of this protocol are similar to procedures of operation of HDLC in "Balanced operation (point-to-point)" mode.

Both connected stations are equal, both are able to set up the data link, disconnect it and send and receive commands and responses without having to get explicit permission from the other station.

Each station has the following:
  
  - variable for remembering the mode
  - status variables
    - send count variable
    - last acknowledged frame variable
    - receive count variable
    - failure count variable
  - internal timer

There are two modes the station can be in:

  - disconnected mode
  - connected mode

### Procedures of operation in disconnected mode

In this mode the station considers the data link to be disconnected. It shall respond with DISCONNECTED frame to all received frames except the CONN frame. It can decide to initialize the connection at any time.

#### Initializing the connection

When the station wishes to initiate a connection it shall send a CONN frame and start the internal timer. When the other station receives a CONN frame it shall reset its status variables, and send CONN_ACK frame as a response. The initializing station, upon reception of the CONN_ACK shall reset its status variables, stop the internal timer and transition to a connected mode. If the internal timer expires, the initializing station may again decide to retry the whole procedure or abandon the attempt.

Case when the CONN_ACK frame is lost is described in "Procedures of operation in connected mode", since the receiving station transitions to the connected mode the moment it transmits CONN_ACK frame.

#### Simultaneous attempts to initialize the connection

If the station sends a CONN frame, and before receiving an appropriate response receivers either DISC frame or CONN frame a link initialization race condition has developed. This situation is described in ISO/IEC 13239:2002(E), section 6.12.4.1.4 (Simultaneous attempts to set mode (contention)). However, HDLC supports setting different modes, whereas we support only one, therefore the solution is simple:

If the station sends CONN frame and receives CONN frame as a response, it shall send CONN_ACK frame, reset its status variables and transition to connected state immediately.

### Procedures of operation in connected mode

In connected mode the stations may exchange data.

#### Handling CONN_ACK frame

If we are in the connected mode and we receive CONN_ACK frame it means that there was a link initialization race condition, and this frame shall be ignored.

#### Handling CONN frame

If we are in the connected mode and we receive CONN frame it means that the other station thinks the link is in disconnected mode. Therefore we shall immediatelly transition to disconnected mode and behave as if we've received CONN frame in disconnected mode.

#### Handling DISCONNECTED frame

If we are in the connected mode and we receive DISCONNECTED frame it means that we've sent some data to the other station, but the other station is in disconnected mode. We must therefore immediately transition to disconnected mode and may decide to initialize the connection again.

#### Exchange of DATA frames

Each sent DATA frame must be acknowledged by the receiving station before the transmitting station sends another DATA frame. Each operation on status variables of the station is modulo 8.

##### Sending DATA frames

When the station wishes to send DATA frame it shall set N(S) to value of the send count variable, N(R) to the value of receive count variable, transmit the frame, store the frame in memory, increment the send count variable and start its internal timer.

The station may not send the DATA frame if it has not yet received DATA_ACK frame acking the previously sent DATA frame (and therefore 'last acknowledged frame variable' != 'send count variable').

##### Receiving DATA_ACK frames

When the station receives DATA_ACK frame and N(R) of that frame matches 'last acknowledged frame variable' it shall treat the previously sent DATA frame as acknowledged and may remove it from memory. The station shall stop its internal timer, increment the last acknowledged frame variable and set the failure count variable to 0.

Otherwise the station shall ignore the DATA_ACK frame.

##### Receiving DATA_NACK frames

When the station receives DATA_NACK frame it shall retransmit the last DATA frame stored in memory, restart its internal timer and increment the failure count variable.

If the internal failure count variable is equal to 10 the station shall transition to disconnected mode and may attempt to reestablish the link.

##### Receiving DATA frames

When the station properly receives DATA frame and N(S) of the frame is equal to the receive count variable the station shall respond with DATA_ACK frame with N(R) set to value of the receive count variable and it shall increment the receive count variable.

When the station properly receives DATA frame and N(S) of that frame is equal to the ('receive count variable' - 1) the DATA_ACK packet probably got lost, therefore the station shall transmit DATA_ACK packet with N(R) set to ('receive count variable' - 1).

In any other case the station shall transition to disconnected mode and may attempt to reestablish the connection.

When the station receives an improper DATA frame it shall respond with DATA_NACK packet with N(R) set to receive count variable.

##### Recovering from time-out errors

When the internal timer of the station sending a DATA frame expires it shall behave as if it has received a DATA_NACK frame.
