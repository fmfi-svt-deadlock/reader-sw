Application layer
=================

Application layer of the Reader <-> Controller protocol has several very simple tasks:

  - Inform the controller that user ID has been obtained (e.g. by reading a card)
  - Inform the controller that an error has occured
  - Inform the reader of a change in the state of the door

Format of messages
------------------

Reader and controller communicate by exchanging Controller-Reader Protocol Messages (CRPM). CRPM has the following format:

```eval_rst

================ =========== =============
 Proto. version   CRPM type   Data
================ =========== =============
 8 bit            8 bit       0 - n bytes
================ =========== =============

```

where

  - Proto. version is the protocol version. In this version, the value is fixed to 1.
  - CRPM type is the type of the message.
  - Data fields contains data specific for the message. Each message specifies structure of the data field.

Each CRPM must fit into a DATA frame of the upper layer and therefore length of the CPRM must not exceed 249 bytes.

Message types
-------------

### ID obtained

```eval_rst

+--------------------------+-------------------------------------------------+
| Message name             | ID obtained                                     |
+--------------------------+-------------------------------------------------+
| CPRM type                | 1                                               |
+--------------------------+-------------------------------------------------+
| Message direction        | Reader -> Controller                            |
+--------------------------+-------------------------------------------------+
| Description              | The Reader has obtained one or more user IDs.   |
+--------------------------+-------------------------------------------------+

```

Data field structure:

```eval_rst

=============== ========== ================ =====
 Number of IDs   ID 0 len   ID 0             ...
=============== ========== ================ =====
 1 byte          1 byte     ID 0 len bytes   ...
=============== ========== ================ =====

```

Where

  - Number of IDs (n) is the number of IDs obtained from the user and carried by this CRPM.
  - This will be followed by n IDs of the following format:
      + ID n len (x) length of the n-th ID
      + ID n - n-th ID

Data field must not exceed ('maximum CRPM size' - 2)

### Reader failure

```eval_rst

+--------------------------+-------------------------------------------------+
| Message name             | Reader failure detected                         |
+--------------------------+-------------------------------------------------+
| CPRM type                | 2                                               |
+--------------------------+-------------------------------------------------+
| Message direction        | Reader -> Controller                            |
+--------------------------+-------------------------------------------------+
| Description              | The Reader has detected an internal error.      |
+--------------------------+-------------------------------------------------+

```

Data field is an ASCII NULL-terminated string describing the error.

### UI update

```eval_rst

+--------------------------+-------------------------------------------------+
| Message name             | UI update                                       |
+--------------------------+-------------------------------------------------+
| CPRM type                | 3                                               |
+--------------------------+-------------------------------------------------+
| Message direction        | Controller -> Reader                            |
+--------------------------+-------------------------------------------------+
| Description              | The Controller wishes to change state of the UI |
+--------------------------+-------------------------------------------------+

```

Controller sends this packet when something the user should be made aware of happens, like door unlocking, system failure...

Interpretation and mapping of these values to specific actions of th UI is responsibility of the Reader.

Data field 1 byte long and can have one of the following values

 - 0: Door is locked
 - 1: ID accepted, door unlocked
 - 2: ID rejected
 - 3: Door permanently unlocked
 - 4: Door permanently locked
 - 5: System failure
 - 6: Door open for too long
 - 47: Vader
