# wisblock-lorawan

Arduino LoRaWAN client for [RAKwireless WisBlock RAK4631](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK4631/Overview/).

Read the article...

["Build a LoRaWAN Network with RAKwireless WisGate Developer Gateway"](https://lupyuen.github.io/articles/wisgate)

[Watch the demo video on YouTube](https://youtu.be/xdyi6XCo8Z8)

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
    printf("RadioSend: size=%d, channel=%d, datarate=%d, txpower=%d, maxeirp=%d, antennagain=%d\r\n", (int) LoRaMacBufferPktLen, (int) txConfig.Channel, (int) txConfig.Datarate, (int) txConfig.TxPower, (int) txConfig.MaxEirp, (int) txConfig.AntennaGain);
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
SX126xSetTxParams: power=0, rampTime=4
SX126xSetPaConfig: paDutyCycle=4, hpMax=7, deviceSel=0, paLut=1 
RadioSetModem
RadioSetModem
RadioSetChannel: freq=923200000
RadioSetTxConfig: modem=1, power=13, fdev=0, bandwidth=0, datarate=10, coderate=1, preambleLen=8, fixLen=0, crcOn=1, freqHopOn=0, hopPeriod=0, iqInverted=0, timeout=3000
RadioSetTxConfig: SpreadingFactor=10, Bandwidth=4, CodingRate=1, LowDatarateOptimize=0, PreambleLength=8, HeaderType=0, PayloadLength=255, CrcMode=1, InvertIQ=0
RadioStandby
RadioSetModem
SX126xSetTxParams: power=13, rampTime=2
SX126xSetPaConfig: paDutyCycle=4, hpMax=7, deviceSel=0, paLut=1 
RadioSetModem
<LMH> Selected subband 1
Joining LoRaWAN network...
RadioSetModem
RadioSetChannel: freq=923400000
RadioSetTxConfig: modem=1, power=13, fdev=0, bandwidth=0, datarate=10, coderate=1, preambleLen=8, fixLen=0, crcOn=1, freqHopOn=0, hopPeriod=0, iqInverted=0, timeout=3000
RadioSetTxConfig: SpreadingFactor=10, Bandwidth=4, CodingRate=1, LowDatarateOptimize=0, PreambleLength=8, HeaderType=0, PayloadLength=0, CrcMode=1, InvertIQ=0
RadioStandby
RadioSetModem
SX126xSetTxParams: power=13, rampTime=2
SX126xSetPaConfig: paDutyCycle=4, hpMax=7, deviceSel=0, paLut=1 
RadioSend: size=23, channel=1, datarate=2, txpower=0, maxeirp=16, antennagain=2
00 00 00 00 00 00 00 00 00 5b b1 7b 37 e7 5e c1 4b 36 65 61 49 88 07 
RadioSend: PreambleLength=8, HeaderType=0, PayloadLength=23, CrcMode=1, InvertIQ=0
<LM> OnRadioTxDone
<LM> OnRadioTxDone => RX Windows #1 5002 #2 6002
<LM> OnRadioTxDone => TX was Join Request
RadioSetChannel: freq=923400000
RadioSetModem
<LM> OnRadioRxDone
<LM> OnRadioRxDone => FRAME_TYPE_JOIN_ACCEPT
OTAA Mode, Network Joined!
Sending frame now...
RadioSetChannel: freq=923400000
RadioSetTxConfig: modem=1, power=13, fdev=0, bandwidth=0, datara, hopPeriod=0, iqInverted=0, timeout=3000
RadioSetTxConfig: SpreadingFactor=10, Bandwidth=4, CodingRate=1, LowDatarateOptimize=0, ertIQ=0
RadioStandby
RadioSetModem
dLength=72, CrcMode=1, SX126xSetTxParams: power=13, rampTime=2
SX126xSetPaConfig: paDutyCycle=4, hpMax=7, deviceSel=0, paLut=1 
RadioSend: size=19, channel=1, datarate=2, txpower=0, maxeirp=16, antennagain=2
40 fc 58 6d 00 80 00 00 02 10 ba 00 65 a5 2a bd ed 36 27 
RadioSend: PreambleLength=8, HeaderType=0, PayloadLength=19, CrcMode=1, InvertIQ=0
lmh_send ok count 1
<LM> OnRadioTxDone
<LM> OnRadioTxDone => RX Windows #1 1002 #2 2002
RadioSetChannel: freq=923400000
RadioSetModem
<RADIO> RadioIrqProcess => IRQ_RX_TX_TIMEOUT
<LM> OnRadioRxTimeout
Sending frame now...
RadioSetChannel: freq=923400000
RadioSetTxConfig: modem=1, power=13, fdev=0, bandwidth=0, datarate=10, coderate=1, preambleLen=8, fixLen=0, crcOn=1, freqHopOn=0, hopPeriod=0, iqInverted=0, timeout=3000
RadioSetTxConfig: SpreadingFactor=10, Bandwidth=4, CodingRate=1, LowDatarateOptimize=0, PreambleLength=8, HeaderType=0, PayloadLength=72, CrcMode=1, InvertIQ=0
RadioStandby
RadioSetModem
SX126xSetTxParams: power=13, rampTime=2
SX126xSetPaConfig: paDutyCycle=4, hpMax=7, deviceSel=0, paLut=1 
RadioSend: size=19, channel=1, datarate=2, txpower=0, maxeirp=16, antennagain=2
40 fc 58 6d 00 80 01 00 02 ea bf 1a 96 e7 4d 76 eb 2c dd 
RadioSend: PreambleLength=8, HeaderType=0, PayloadLength=19, CrcMode=1, InvertIQ=0
lmh_send ok count 2
<LM> OnRadioTxDone
<LM> OnRadioTxDone => RX Windows #1 1002 #2 2002
RadioSetChannel: freq=923400000
RadioSetModem
<RADIO> RadioIrqProcess => IRQ_RX_TX_TIMEOUT
<LM> OnRadioRxTimeout
Sending frame now...
RadioSetChannel: freq=924600000
RadioSetTxConfig: modem=1, power=13, fdev=0, bandwidth=0, datarate=10, coderate=1, preambleLen=8, fixLen=0, crcOn=1, freqHopOn=0, hopPeriod=0, iqInverted=0, timeout=3000
RadioSetTxConfig: SpreadingFactor=10, Bandwidth=4, CodingRate=1, LowDatarateOptimize=0, PreambleLength=8, HeaderType=0, PayloadLength=72, CrcMode=1, InvertIQ=0
RadioStandby
RadioSetModem
SX126xSetTxParams: power=13, rampTime=2
SX126xSetPaConfig: paDutyCycle=4, hpMax=7, deviceSel=0, paLut=1 
RadioSend: size=19, channel=7, datarate=2, txpower=0, maxeirp=16, antennagain=2
40 fc 58 6d 00 80 02 00 02 83 7b e1 dc cc f3 84 89 b9 e8 
RadioSend: PreambleLength=8, HeaderType=0, PayloadLength=19, CrcMode=1, InvertIQ=0
lmh_send ok count 3
<LM> OnRadioTxDone
<LM> OnRadioTxDone => RX Windows #1 1002 #2 2002
RadioSetChannel: freq=924600000
RadioSetModem
<RADIO> RadioIrqProcess => IRQ_RX_TX_TIMEOUT
<LM> OnRadioRxTimeout
```
