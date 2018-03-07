Task
====

The Deadlock Reader firmware is composed of several Tasks and a signle Master Task. Each task will
run in its separate thread and will do only one thing, for example there will be a task for reading
the RFID card or task for updating the UI. These tasks won't communicate directly with each other.
They will only communicate with the Master Task. The Master Task will implement business logic
of the reader.

Tasks are located in subfolders `src/tasks`, one folder per task. Each task has a header, which
defines an interface for that task.

To lower implementation and maintenance costs, there can only be a single instance of each task.
Task interface functions therefore operate on a global instance of the given task.


Task states
-----------

The following state machine represents an abstract task:

```eval_rst

.. graphviz::

    digraph G {
        uninitialized -> stopped [label="dlTaskInit()"]
        stopped -> running [label="dlTaskStart()"]
        running -> stopped [label="dlTaskStop() or task exited"]
    }

```

  - `uninitialized`: internal structures of the task are not initialized. The task can't be
    started and no task API functions are callable. The task won't call Master Task callbacks.
  - `stopped`: internal task structures are now initialized and the task can be stared. No task
    APIs can be called and the task won't call Master Task callbacks.
  - `running`: the task is functioning normally (the task thread is running). Task API is available
    and the task will call Master Task callbacks. In particular, each task is required to
    periodically call Heartbeat callback.

All Tasks implement this state machine. The Master Task is responsible for initializing, starting
(and if required stopping) Tasks.

The Master Task also implements this state machine. The Master Task is initialized and started
by the firmware initialization code. This code just starts the task, it does not monitor it and
it won't restart it if the task transitions back to `stopped` state.


Task API and naming conventions
-------------------------------

Naming conventions of the API are similar to ChibiOS naming conventions:
http://chibios.sourceforge.net/docs3/rt/concepts.html#naming

The API functions are following this convention: `dl<group><subgroup><action>()`

  - `dl`: stands for Deadlock
  - `group`: for tasks API, the Group is always `Task`
  - `subgroup`: subgroup is the name of the task
  - `action`: what the API function does (`Start`, `Stop`, …)

Examples: `dlTaskUIStart()` or `dlTaskRFIDStopPolling()`.

Each task must provide functions used in the state machine transitions:

  - `dlTask<TaskName>Init()` - initializes task data structures and prepares it to be started
  - `dlTask<TaskName>Start()` - starts the task thread
  - `dlTask<TaskName>Stop()` - stops the task thread
