# ESP32 Modbus Server Example

In this project we are using the CAS Modbus stack (https://store.chipkin.com) to generate a simple Modbus TCP server with three registers 40001-40003. Depening on the state of the registers the built in LED wil on/off/blink. A Modbus Client/Master application (such as [CAS Modbus Scanner](https://store.chipkin.com/products/tools/cas-modbus-scanner)) can be used to read/write to these registers and to change the values.

| Register | Description                           | Default |
| :------- | ------------------------------------- | ------: |
| 40001    | LED Mode (1 = Off, 2 = On, 3 = Blink) |       3 |
| 40002    | LED Blink Speed in milliseconds       |     200 |
| 40003    | LED Current state (0 = OFF, 1 = ON)   |       0 |
| 40004    | Running Count. This value incurments once a second and loops every ~18 hours. | 0 |
| 40005    | Spare | 0 |
| 40006    | Spare | 0 |
| 40007    | Spare | 0 |
| 40008    | Spare | 0 |
| 40009    | Spare | 0 |

## Quick start

1. Download and install [Platform/io](https://platformio.org/) for [Visual studios code](https://code.visualstudio.com/)
2. Add the ESP32 board to Platform/io
3. Add the [CAS Modbus stack](https://store.chipkin.com/) libary to the */lib/* folder.
4. Use Platform/io to *Build* the project.
5. Use Platform/io to *Upload and Monitor*.

## Supported Modbus functions

The CAS Modbus stack supports many other Modbus functions, this minumial example only supports the following:

- 03 (0x03) **Read Holding Registers**
- 06 (0x06) **Write Single Register**
- 16 (0x10) **Write Multiple registers**

## Example output from CAS Modbus Scanner

The following output was produced using the CAS Modbus Scanner when reading values from the ESP32-ModbusServerExample.

```txt
Poll: FF 03 00 00 00 09
Response: 01 03 12 00 03 00 FA 00 01 90 5A 00 00 00 00 00 00 00 00 00 00
```

| Register | Value (Hex) | Value (Uint16) |
| :------- | ----------: | -------------: |
| 40001    |      0x0003 |              3 |
| 40002    |      0x00FA |            250 |
| 40003    |      0x0001 |              1 |
| 40004    |      0x905A |          36954 |
| 40005    |      0x0000 |              0 |
| 40006    |      0x0000 |              0 |
| 40007    |      0x0000 |              0 |
| 40008    |      0x0000 |              0 |
| 40009    |      0x0000 |              0 |

## Example in the CAS Modbus Scanner

![Preview of the CAS Modbus Scanner](/docs/CASModbusScanner.png?raw=true "Preview of the CAS Modbus Scanner")

## Example output

The example output from the serial terminal of the ESP32

```txt
FYI: ESP32 CAS Modbus Stack example version: 0.0.1
FYI: ESP32 Chip ID: 30C6, (08A4AE30)
FYI: Connecting to wifi...
FYI: Connected to XXXXXXXXXXXX
FYI: IP address: 192.168.1.66
FYI: Subnet mask: 255.255.255.0
FYI: CAS Modbus Stack version: 2.3.11
FYI: FreeHeap: 280012 / 343144 (81.60 %)
FYI: New TCP connection
FYI: RemoteIP: 192.168.1.77, Port: 27484
FYI: RecvModbusMessage bytes=[12]
FYI: GetModbusValue slaveAddress=[1], function=[3], startingAddress=[0], length=[9]
   40001 = 3 (0x0003)
   40002 = 250 (0x00FA)
   40003 = 1 (0x0001)
   40004 = 36954 (0x905A)
   40005 = 0 (0x0000)
   40006 = 0 (0x0000)
   40007 = 0 (0x0000)
   40008 = 0 (0x0000)
   40009 = 0 (0x0000)
FYI: sendModbusMessage bytes=[27]
FYI: FreeHeap: 277328 / 342900 (80.88 %)
-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

## Tested hardware

- [Adafruit HUZZAH32 – ESP32 Feather Board](https://www.adafruit.com/product/3405)

## FAQ

### Can't build the project. *CASModbusAdapter.h: No such file or directory*

Please see this issue [CASBACnetStackAdapter.h: No such file or directory](https://github.com/chipkin/ESP32-ModbusServerExample/issues/1)