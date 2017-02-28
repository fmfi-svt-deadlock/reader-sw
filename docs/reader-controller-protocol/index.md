Reader <-> Controller Protocol
==============================

This document describes all layers of the communication protocol between `basic-isic-reader-revA` and the Controller.

TODO find a better place for this.

Motivation
----------

In the old concept of Deadlock there was a custom-designed communication protocol. This protocol had a homebrew (and therefore probably unreliable) "lower layer" which was unbalanced, therefore the master (Controller) had to continuously poll the Reader for status. This was inefficient. Also the application layer was just for several basic UI commands and for encapsulation of SPI commands sent directly to the MRFC522 module. This goes against the current philosophy of the Reader. Clearly both needed to be redesigned.

The physical layer was kept mostly as-is. The only change was transition from 3.3V UART signalling to RS-232 voltage levels to improve signal integrity.

The link, network and transport layer should provide reliable transmission of datagrams over a balanced link. Since this is a solved problem it makes no sense to come up with a completely custom protocol. [HDLC (High-Level Data Link Control)](https://en.wikipedia.org/wiki/High-Level_Data_Link_Control) already does what I need. And I've found an implementation which looks solid enough: the [yahdlc](https://github.com/bang-olufsen/yahdlc). It is not exactly HDLC, it is a very small subset, but it does almost exactly what I need. It even has [Python bindings](https://github.com/SkypLabs/python4yahdlc). But there is a small detail that needs to be fixed: the library provides no way to reset the link, wihch is important for me, since the watchdog may reset any of the connected devices at any time and the link must recover. So I'll very slightly modify that library to add capability to connect and disconnect the data link (trying to be compliant / keep as close as possible to the HDLC spec). Then by disconnecting and reconnecting the link can be reset.

The application layer needed to be designed from scratch anyway, and is quite simple.

Overally, not reusing the old-concept Reader <-> Controller protocol is less work.

Protocol stack description
--------------------------

```eval_rst

.. toctree::
   :maxdepth: 2

   physical_layer
   link_network_transport_layer
   application_layer

```
