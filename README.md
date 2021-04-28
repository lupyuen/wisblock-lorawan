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

# Message Integrity Code Errors

To search for Message Integrity Code errors in LoRaWAN Packets received by WisGate, SSH to WisGate and search for...

```bash
# grep MIC /var/log/syslog

Apr 28 04:02:05 rak-gateway 
chirpstack-application-server[568]: 
time="2021-04-28T04:02:05+01:00" 
level=error 
msg="invalid MIC" 
dev_eui=4bc15ee7377bb15b 
type=DATA_UP_MIC

Apr 28 04:02:05 rak-gateway 
chirpstack-network-server[1378]: 
time="2021-04-28T04:02:05+01:00" 
level=error 
msg="uplink: processing uplink frame error"
ctx_id=0ccd1478-3b79-4ded-9e26-a28e4c143edc 
error="get device-session error: invalid MIC"
```

The error above occurs when we replay a repeated Join Network Request to our LoRaWAN Gateway (with same Nonce, same Message Integrity Code).

This replay also logs a Nonce Error in WisGate...

```bash
# grep nonce /var/log/syslog

Apr 28 04:02:41 rak-gateway chirpstack-application-server[568]:
time="2021-04-28T04:02:41+01:00" 
level=error 
msg="validate dev-nonce error" 
dev_eui=4bc15ee7377bb15b 
type=OTAA

Apr 28 04:02:41 rak-gateway chirpstack-network-server[1378]:
time="2021-04-28T04:02:41+01:00" 
level=error 
msg="uplink: processing uplink frame error" ctx_id=01ae296e-8ce1-449a-83cc-fb0771059d89 
error="validate dev-nonce error: object already exists"
```

Because the Nonce should not be reused.

# Log Transmitted Packets

To log transmitted packets, modify

`.pio/libdeps/wiscore_rak4631/SX126x-Arduino/src/mac/LoRaMac.cpp`

```c
LoRaMacStatus_t SendFrameOnChannel(uint8_t channel)
{
    TxConfigParams_t txConfig;
    int8_t txPower = 0;

    txConfig.Channel = channel;
    txConfig.Datarate = LoRaMacParams.ChannelsDatarate;
    txConfig.TxPower = LoRaMacParams.ChannelsTxPower;
    txConfig.MaxEirp = LoRaMacParams.MaxEirp;
    txConfig.AntennaGain = LoRaMacParams.AntennaGain;
    txConfig.PktLen = LoRaMacBufferPktLen;

    // If we are connecting to a single channel gateway we use always the same predefined channel and datarate
    if (singleChannelGateway)
    {
        txConfig.Channel = singleChannelSelected;
        txConfig.Datarate = singleChannelDatarate;
    }

    RegionTxConfig(LoRaMacRegion, &txConfig, &txPower, &TxTimeOnAir);

    MlmeConfirm.Status = LORAMAC_EVENT_INFO_STATUS_ERROR;
    McpsConfirm.Status = LORAMAC_EVENT_INFO_STATUS_ERROR;
    McpsConfirm.Datarate = LoRaMacParams.ChannelsDatarate;
    McpsConfirm.TxPower = txPower;

    // Store the time on air
    McpsConfirm.TxTimeOnAir = TxTimeOnAir;
    MlmeConfirm.TxTimeOnAir = TxTimeOnAir;

    // Starts the MAC layer status check timer
    TimerSetValue(&MacStateCheckTimer, MAC_STATE_CHECK_TIMEOUT);
    TimerStart(&MacStateCheckTimer);

    if (IsLoRaMacNetworkJoined != JOIN_OK)
    {
        JoinRequestTrials++;
    }

    //////////////// INSERT THIS CODE

    // To replay a Join Network Request...
    // if (LoRaMacBuffer[0] == 0) {
    // 	static uint8_t replay[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5b, 0xb1, 0x7b, 0x37, 0xe7, 0x5e, 0xc1, 0x4b, 0x67, 0xaa, 0xbb, 0x07, 0x70, 0x7d};
    // 	memcpy(LoRaMacBuffer, replay, LoRaMacBufferPktLen);
    // }

    // To dump transmitted packets...
    printf("RadioSend: size=%d\r\n", (int) LoRaMacBufferPktLen);
    for (int i = 0; i < LoRaMacBufferPktLen; i++) {
        printf("%02x ", LoRaMacBuffer[i]);
    }
    printf("\r\n");
    
    //////////////// END OF INSERTION

    // Send now
    Radio.Send(LoRaMacBuffer, LoRaMacBufferPktLen);

    LoRaMacState |= LORAMAC_TX_RUNNING;

    return LORAMAC_STATUS_OK;
}
```

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
