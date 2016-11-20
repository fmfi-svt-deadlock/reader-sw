RFID Stack
==========

Main functionality of the Reader is to, of course, read an RFID card. To accomplish this it uses a modular stack of libraries written for this purpose. Goals of these libs are:

 - To be modular, with clearly defined interfaces
 - To fully comply with international standards
 - To be well-documented and usable even outside of Deadlock
     + Specifically, its structure should resemble other ChibiOS HAL components and should be easily incorporable to ChibiOS Community HAL
 - To be thread safe
 - To be easy to use, but not while compromising modularity

### Documentation of components

```eval_rst
.. toctree::
    :maxdepth: 2

    hal_abstract_iso14443_pcd
    hal_abstract_CRCard
```

Architecture
------------

Currently the plan is to support primarily [ISO/IEC 14443A](http://www.iso.org/iso/home/store/catalogue_tc/catalogue_detail.htm?csnumber=70172)-compatible cards. The following diagram shows the architecture, currently implemented and planned components:

```eval_rst

.. graphviz::

    digraph G {

        rankdir="BT"

        node [
            shape = "record"
        ]
        
        hal_mfrc522 -> hal_abstract_iso14443_pcd [arrowhead="empty"]
        hal_abstract_iso14443_pcd -> hal_iso_14443_picc [arrowhead="open"]
        hal_iso_14443_picc -> hal_abstract_CRCard [arrowhead="empty"]
        hal_abstract_CRCard -> hal_DESFire_card [arrowhead="open"]
        hal_abstract_CRCard -> hal_iso_7816 [arrowhead="open"]
        hal_iso_7816 -> hal_abstract_CRCard [arrowhead="empty" label="APDU Command Wrapping"]
    }

```

Components
----------

Abstract classes:

  - `hal_abstract_iso14443_pcd`: Abstract representation of a RFID module capable of reading ISO/IEC 14443-compliant cards
  - `hal_abstract_CRCard`: Abstract representation of either contactless card or card with contacts which communicates by exchanging Command - Response pairs (where every command generates either an response or a timeout).

Protocol implementations:

  - `hal_iso_14443_picc`: Implements initialization, anticollision and communication protocol with ISO/IEC 14443-compliant card.
  - `hal_iso_7816`: (Planned) implements industry-standard commands and wrapping of other protocols into its APDUs (and therefore creating another instance of `CRCard` where command-response pairs are wrapped in its APDUs)
  - `hal_DESFire_card`: (Planned) implements proprietary DESFire command set.

Device drivers:

  - `hal_mfrc522`: Driver for the MFRC522 module.

Abstract classes in Plain C
---------------------------

The RFID stack is written in plain C. That does not mean it can't be written in an object-oriented way.

"Object" in this case is a structure which contains a pointer to another structure, called a [Virtual Method Table](https://en.wikipedia.org/wiki/Virtual_method_table) and arbitrary other private data. The virtual method table is a structure containing pointers to "member functions". These functions, by convention, expect instance of the "object" structure as a first parameter.

To call a "member function" of an "object" you then just would do the following:

```c
result = obj->vmt->funct(obj, param1, param2);
```

However, this is cumbersome to write, so libraries which define abstract classes also include a convenience macro for each function:

```c
#define funct(obj, param1, param2) ((obj)->vmt->funct(obj, param1, param2))
```

so then you will just write

```c
result = funct(obj, param1, param2)
```

To "extend" such an abstract class you just have to define all functions in the Virtual Method Table and provide a function which instantiates this table and instantiates structure which represents the object:

```c
typedef struct {
    uint8_t accumulator;
} my_privatedata;

// Definition of a member method
result_t my_funct(FunnyObj obj, uint8_t param1, uint8_t param2) {
    ((my_privatedata)(obj->data))->accumulator += param1;
    ((my_privatedata)(obj->data))->accumulator += param2;
    return ((my_privatedata)(obj->data))->accumulator;
}

// Something like a constructor
FunnyObj my_create_instance(uint8_t init_acc) {
    FunnyObj obj = malloc(sizeof(FunnyObj));
    obj->vmt = malloc(sizeof(FunnyObjVMT));
    obj->data = malloc(sizeof(my_privatedata));

    obj->vmt->funct = &my_funct;
    ((my_privatedata)(obj->data))->accumulator = 0;
}
```

Of course you could do some more preprocessor magic, but that would violate the principle of least astonishment. This by itself borders on violating it, however, it is the only way to write truly modular protocol stack which allows for easy component swapping and using multiple components of the same type (e.g. reader drivers) simultaneously.

### Documentation of abstract classes

TODO where? Should the documentation document the defines, or VMT methods?
