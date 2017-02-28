Physical layer
==============

Reader and Controller are physically interconnected using 6-wire cable terminated with RJ12 (6P6C) connectors. The pin assignment is as follows:

```eval_rst

======= ==============================
  Pin    Function                     
======= ==============================
  1      RST                          
  2      Vcc                          
  3      reader RxD (controller TxD)  
  4      reader TxD (controller RxD)  
  5      GND                          
  6      Boot                         
======= ==============================

```

Signalling
----------

GND and Vcc are the power supply pins. The voltage may be anywhere between +4 to +25 volts, and the Controller must have the capability to supply at least 100mA of current at +5V (less at higher voltages).

RST and Boot pins are driven by the Controller. Logical signalling level on those pins is +3.3V. RST is active LOW, idle HIGH. Boot is idle LOW, active HIGH. LOW pulse will generate reset of the Reader. If Boot signal is active during power-up or reset the Reader enters the 'Bootloader mode'. Read documentation for the Reader for more info on the 'Bootloader mode'.

RxD and TxD pins are RS-232 compatible UART communication lines, with voltage levels as specified by RS-232 standard. Used baud rate is 38 400 baud, one end bit, even parity.
