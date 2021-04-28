# wisblock-lorawan

Arduino LoRaWAN client for [RAKwireless WisBlock RAK4630](https://docs.rakwireless.com/Product-Categories/WisBlock/Quickstart/).

https://github.com/RAKWireless/WisBlock/tree/master/examples/RAK4630/communications/LoRa/LoRaWAN

Based on...

https://github.com/RAKWireless/WisBlock/blob/master/examples/RAK4630/communications/LoRa/LoRaWAN/LoRaWAN_OTAA_ABP/LoRaWAN_OTAA_ABP.ino

Note: This program needs SX126x-Arduino Library version 2.0.0 or later. 

In [`platformio.ini`](platformio.ini) set...

```text
lib_deps = beegee-tokyo/SX126x-Arduino@^2.0.0
```

Follow the Twitter Thread...

https://twitter.com/MisterTechBlog/status/1379926160377851910

# Output Log

```text
> Executing task: platformio device monitor <

--- Available filters and text transformations: colorize, debug, default, direct, hexlify, log2file, nocontrol, printable, send_on_enter, time
--- More details at http://bit.ly/pio-monitor-filters
--- Miniterm on /dev/cu.usbmodem14201  9600,8,N,1 ---
--- Quit: Ctrl+C | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---
=====================================
Welcome to RAK4630 LoRaWan!!!
Type: OTAA
Region: AS923
=====================================
<LMH> OTAA 
DevEui=4B-C1-5E-E7-37-7B-B1-5B
DevAdd=00000000
AppEui=00-00-00-00-00-00-00-00
AppKey=AA-FF-AD-5C-7E-87-F6-4D-E3-F0-87-32-FC-1D-D2-5D
<LMH> Selected subband 1
Joining LoRaWAN network...
RadioSend: size=23
00 00 00 00 00 00 00 00 00 5b b1 7b 37 e7 5e c1 4b 55 ed 7e 9b d6 af 
<LM> OnRadioTxDone
<LM> OnRadioTxDone => RX Windows #1 5002 #2 6002
<LM> OnRadioTxDone => TX was Join Request
<LM> OnRadioRxDone
<LM> OnRadioRxDone => FRAME_TYPE_JOIN_ACCEPT
OTAA Mode, Network Joined!
Sending frame now...
RadioSend: size=19
40 4a f9 43 00 80 00 00 02 ba 3f 06 65 98 72 bc a2 27 0b 
lmh_send ok count 1
<LM> OnRadioTxDone
<LM> OnRadioTxDone => RX Windows #1 1002 #2 2002
<RADIO> RadioIrqProcess => IRQ_RX_TX_TIMEOUT
<LM> OnRadioRxTimeout
Sending frame now...
RadioSend: size=19
40 4a f9 43 00 80 01 00 02 b9 97 59 b5 bb b1 00 f1 88 36 
lmh_send ok count 2
<LM> OnRadioTxDone
<LM> OnRadioTxDone => RX Windows #1 1002 #2 2002
<RADIO> RadioIrqProcess => IRQ_RX_TX_TIMEOUT
<LM> OnRadioRxTimeout
Sending frame now...
RadioSend: size=19
40 4a f9 43 00 80 02 00 02 24 9f 11 8d 4f 78 a7 67 49 18 
lmh_send ok count 3
<LM> OnRadioTxDone
<LM> OnRadioTxDone => RX Windows #1 1002 #2 2002
<RADIO> RadioIrqProcess => IRQ_RX_TX_TIMEOUT
<LM> OnRadioRxTimeout
```
