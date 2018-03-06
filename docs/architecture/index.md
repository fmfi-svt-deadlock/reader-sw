Reader Firmware Architecture
============================

The Reader uses STM32 microcontroller. To keep the cost of hardware as low as possible, these
MCUs are not capable of running mainstream OS such as Linux. Since the Reader must be fast and
stable, the firmware must be written in a way that it can execute efficiently on an MCU - therefore
in C.

Writing a firmware in C for bare-metal, however, has several disadvantages. One big disadvantage
is portability: should the Reader require hardware change the firmware would have to be laboriously
ported as well, including possible rewrite of bare-metal drivers. Another disadvantage is high
maintenance cost and entry barrier for new developers entering the project.

To mitigate some of these disadvantages, we have decide to base the firmware on ChibiOS. ChibiOS
is a free development environment for embedded applications. It contains a RTOS, which provides
threading capabilities, synchronization primitives and other useful programming constructs, and
HAL, which simplifies development and facilitates firmware porting to different MCUs.

There are multiple ways to write a firmware for embedded devices, from a simple loop to extremely
modular but complex microkernel-like designs. Each design method provides a different mix of
implementation cost, maintainability, stability and extensibility.


Components and their interaction
--------------------------------

Firmware of the Reader will be composed of several Tasks and one Master Task. Each task will run
in its separate thread and will do only one thing, for example there will be a task for reading the
RFID card or task for updating the UI. These tasks won't communicate directly with each other.
They will only communicate with the Master Task. The Master Task will implement business logic
of the reader.

Since Tasks and the Master Task run in different threads, some form of inter-thread communication
will have to be set up. In general, tasks will provide a thread-safe API which the Master Task
may call whenever it needs. This thread-safe API will internally, in a task-specific way, transport
the message to the task thread. This is done so that each task may choose communication method best
suited for it. For example, RFID Card Task may support only 2 commands: Start Polling
and Stop Polling. The simplest way to implement these commands is to just set a bit in shared
memory, update of which is always atomic. On the other hand, the Communicator Task will have to
receive a data structure describing message the Master Task wishes to send. The Master Task may
even request sending a message if another message is being sent just now. Most fitting communication
mechanism for this particular task is the Mailbox construct.


Watchdog
--------

The multithreaded nature of this firmware also brings some issues. Since each task runs as a
separate thread, it may lock up / enter an endless loop without impairing the rest of the system.
For example, RFID Card Task may enter an endless loop. Since other threads, including the Master
Task may run with higher priority, they won't notice anything abnormal, except that when user
presents a card to the reader, the card won't be detected. Therefore it is necessary to implement
a watchdog which would watch over all tasks (including the Master Task), and if something like this
happens, it would reset the Reader.

At this point one may be tempted to come up with a complex scheme where each task can be restarted
without affecting any other tasks. Indeed, the first working draft of the firmware design included
this capability. Implementation, however, proved to be quite challenging, and would require
implementation of some sort of garbage-collecting mechanism. In the end, we have decided that added
implementation complexity and potential bugs were not worth the advantages such mechanism would
bring. Indeed, the whole Reader firmware is able to fully boot up in a fraction of a second after
reset, so it is easier to just go for a complete restart.

The implemented watchdog mechanism utilizes the hardware watchdog built in the MCU. The watchdog
in this MCU is essentially just a timer, which when runs out, resets the MCU. So the firmware
has to perpetually reset this timer. Resetting the watchdog is the responsibility of the Master
Task.

Each task, including a Master Task, should generate Heartbeats. This Heartbeat should depend on
correct operation of the given Task. For example, the RFID Card Task tries to read a card in a loop.
After each loop, it can generate a Heartbeat. If the task gets stuck when reading a card, it won't
be able to generate a Heartbeat.

The Master Task internally keeps a vector of bits, where each bit represents a task (Heartbeat
Vector). Each task sends a heartbeat message to the Master Task each time the task generates a
Heartbeat. When the Master Task receives that message, it sets a bit in the Heartbeat Vector.
Then, when the Master Task itself generates a Heartbeat, it checks the Heartbeat Vector, and if
all bits are set, it resets the Watchdog, and clears all bits in the Heartbeat Vector. Therefore if
some tasks continually misses its Heartbeats, its bit won't ever be set and the Watchdog won't be
reset.


Detailed descriptions
---------------------

```eval_rst
.. toctree::
    :maxdepth: 2

    tasks/task
    tasks/ui-task
```
