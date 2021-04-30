//  Based on https://github.com/RAKWireless/WisBlock/blob/master/examples/RAK4630/communications/LoRa/LoRaWAN/LoRaWAN_OTAA_ABP/LoRaWAN_OTAA_ABP.ino
//  Note: This program needs SX126x-Arduino Library version 2.0.0 or later. In platformio.ini, set...
//    lib_deps = beegee-tokyo/SX126x-Arduino@^2.0.0
/**
 * @file LoRaWAN_OTAA_ABP.ino
 * @author rakwireless.com
 * @brief LoRaWan node example with OTAA/ABP registration
 * @version 0.1
 * @date 2020-08-21
 * 
 * @copyright Copyright (c) 2020
 * 
 * @note RAK4631 GPIO mapping to nRF52840 GPIO ports
   RAK4631    <->  nRF52840
   WB_IO1     <->  P0.17 (GPIO 17)
   WB_IO2     <->  P1.02 (GPIO 34)
   WB_IO3     <->  P0.21 (GPIO 21)
   WB_IO4     <->  P0.04 (GPIO 4)
   WB_IO5     <->  P0.09 (GPIO 9)
   WB_IO6     <->  P0.10 (GPIO 10)
   WB_SW1     <->  P0.01 (GPIO 1)
   WB_A0      <->  P0.04/AIN2 (AnalogIn A2)
   WB_A1      <->  P0.31/AIN7 (AnalogIn A7)
 */
#include <Arduino.h>
#include <LoRaWan-RAK4630.h> //http://librarymanager/All#SX126x
#include <SPI.h>

// RAK4630 supply two LED
#ifndef LED_BUILTIN
#define LED_BUILTIN 35
#endif

#ifndef LED_BUILTIN2
#define LED_BUILTIN2 36
#endif

//  TODO: Set this to your LoRaWAN Region
LoRaMacRegion_t g_CurrentRegion = LORAMAC_REGION_AS923;

//  Set to true to select Over-The-Air Activation (OTAA), false for Activation By Personalisation (ABP)
bool doOTAA = true;

//  TODO: (For OTAA Only) Set the OTAA keys. KEYS ARE MSB !!!!

//  Device EUI: Copy from ChirpStack: Applications -> app -> Device EUI
uint8_t nodeDeviceEUI[8] = {0x4b, 0xc1, 0x5e, 0xe7, 0x37, 0x7b, 0xb1, 0x5b};

//  App EUI: Not needed for ChirpStack, set to default 0000000000000000
uint8_t nodeAppEUI[8]    = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

//  App Key: Copy from ChirpStack: Applications -> app -> Devices -> device_otaa_class_a -> Keys (OTAA) -> Application Key
uint8_t nodeAppKey[16]   = {0xaa, 0xff, 0xad, 0x5c, 0x7e, 0x87, 0xf6, 0x4d, 0xe3, 0xf0, 0x87, 0x32, 0xfc, 0x1d, 0xd2, 0x5d};

//  TODO: (For ABP Only) Set the ABP keys
uint32_t nodeDevAddr    = 0x260116F8;
uint8_t nodeNwsKey[16]  = {0x7E, 0xAC, 0xE2, 0x55, 0xB8, 0xA5, 0xE2, 0x69, 0x91, 0x51, 0x96, 0x06, 0x47, 0x56, 0x9D, 0x23};
uint8_t nodeAppsKey[16] = {0xFB, 0xAC, 0xB6, 0x47, 0xF3, 0x58, 0x45, 0xC7, 0x50, 0x7D, 0xBF, 0x16, 0x8B, 0xA8, 0xC1, 0x7C};

#define SCHED_MAX_EVENT_DATA_SIZE APP_TIMER_SCHED_EVENT_DATA_SIZE /**< Maximum size of scheduler events. */
#define SCHED_QUEUE_SIZE 60										  /**< Maximum number of events in the scheduler queue. */
#define LORAWAN_DATERATE DR_0									  /*LoRaMac datarates definition, from DR_0 to DR_5*/
#define LORAWAN_TX_POWER TX_POWER_5							      /*LoRaMac tx power definition, from TX_POWER_0 to TX_POWER_15*/
#define JOINREQ_NBTRIALS 3										  /**< Number of trials for the join request. */
DeviceClass_t g_CurrentClass = CLASS_A;					          /* class definition*/
lmh_confirm g_CurrentConfirm = LMH_UNCONFIRMED_MSG;				  /* confirm/unconfirm packet definition*/
uint8_t gAppPort = LORAWAN_APP_PORT;							  /* data port*/

/**@brief Structure containing LoRaWan parameters, needed for lmh_init()
*/
static lmh_param_t g_lora_param_init = {
    LORAWAN_ADR_ON, 
    LORAWAN_DATERATE, 
    LORAWAN_PUBLIC_NETWORK, 
    JOINREQ_NBTRIALS, 
    LORAWAN_TX_POWER, 
    LORAWAN_DUTYCYCLE_OFF
};

// Foward declaration
static void lorawan_has_joined_handler(void);
static void lorawan_join_failed_handler(void);
static void lorawan_rx_handler(lmh_app_data_t *app_data);
static void lorawan_confirm_class_handler(DeviceClass_t Class);
static void send_lora_frame(void);

/**@brief Structure containing LoRaWan callback functions, needed for lmh_init()
*/
static lmh_callback_t g_lora_callbacks = {
    BoardGetBatteryLevel, 
    BoardGetUniqueId, 
    BoardGetRandomSeed,
    lorawan_rx_handler, 
    lorawan_has_joined_handler, 
    lorawan_confirm_class_handler, 
    lorawan_join_failed_handler
};

// Private defination
#define LORAWAN_APP_DATA_BUFF_SIZE 64                     /**< buffer size of the data to be transmitted. */
#define LORAWAN_APP_INTERVAL 20000                        /**< Defines for user timer, the application data transmission interval. 20s, value in [ms]. */
static uint8_t m_lora_app_data_buffer[LORAWAN_APP_DATA_BUFF_SIZE];            //< Lora user application data buffer.
static lmh_app_data_t m_lora_app_data = {m_lora_app_data_buffer, 0, 0, 0, 0}; //< Lora user application data structure.

TimerEvent_t appTimer;
static uint32_t timers_init(void);
static uint32_t count = 0;
static uint32_t count_fail = 0;

// At startup, we join the LoRaWAN network, in either OTAA or ABP mode
void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Initialize LoRa chip.
  lora_rak4630_init();

  // Initialize Serial for debug output
  time_t timeout = millis();
  Serial.begin(115200);
  while (!Serial)
  {
    if ((millis() - timeout) < 5000)
    {
      delay(100);
    }
    else
    {
      break;
    }
  }
  Serial.println("=====================================");
  Serial.println("Welcome to RAK4630 LoRaWan!!!");
  if (doOTAA)
  {
    Serial.println("Type: OTAA");
  }
  else
  {
    Serial.println("Type: ABP");
  }

  switch (g_CurrentRegion)
  {
    case LORAMAC_REGION_AS923:
      Serial.println("Region: AS923");
      break;
    case LORAMAC_REGION_AU915:
      Serial.println("Region: AU915");
      break;
    case LORAMAC_REGION_CN470:
      Serial.println("Region: CN470");
      break;
    case LORAMAC_REGION_EU433:
      Serial.println("Region: EU433");
      break;
    case LORAMAC_REGION_IN865:
      Serial.println("Region: IN865");
      break;
    case LORAMAC_REGION_EU868:
      Serial.println("Region: EU868");
      break;
    case LORAMAC_REGION_KR920:
      Serial.println("Region: KR920");
      break;
    case LORAMAC_REGION_US915:
      Serial.println("Region: US915");
      break;
  }
  Serial.println("=====================================");
  
  //creat a user timer to send data to server period
  uint32_t err_code;
  err_code = timers_init();
  if (err_code != 0)
  {
    Serial.printf("timers_init failed - %d\n", err_code);
    return;
  }

  // Setup the EUIs and Keys
  if (doOTAA)
  {
    lmh_setDevEui(nodeDeviceEUI);
    lmh_setAppEui(nodeAppEUI);
    lmh_setAppKey(nodeAppKey);
  }
  else
  {
    lmh_setNwkSKey(nodeNwsKey);
    lmh_setAppSKey(nodeAppsKey);
    lmh_setDevAddr(nodeDevAddr);
  }

  // Initialize LoRaWaN
  err_code = lmh_init(  //  lmh_init now takes 3 parameters instead of 5
    &g_lora_callbacks,  //  Callbacks 
    g_lora_param_init,  //  Functions
    doOTAA,             //  Set to true for OTAA
    g_CurrentClass,     //  Class 
    g_CurrentRegion     //  Region
  );
  if (err_code != 0)
  {
    Serial.printf("lmh_init failed - %d\n", err_code);
    return;
  }

  // Start Join procedure
  Serial.println("Joining LoRaWAN network...");
  lmh_join();
}

// Nothing here for now
void loop()
{
  // Put your application tasks here, like reading of sensors,
  // Controlling actuators and/or other functions. 
}

/**@brief LoRa function for handling HasJoined event.
 */
void lorawan_has_joined_handler(void)
{
  // When we have joined the LoRaWAN network, start a 20-second timer
  Serial.println("OTAA Mode, Network Joined!");

  lmh_error_status ret = lmh_class_request(g_CurrentClass);
  if (ret == LMH_SUCCESS)
  {
    delay(1000);
    TimerSetValue(&appTimer, LORAWAN_APP_INTERVAL);
    TimerStart(&appTimer);
  }
}

/**@brief LoRa function for handling OTAA join failed
*/
static void lorawan_join_failed_handler(void)
{
  // If we can't join the LoRaWAN network, show the error
  Serial.println("OTAA join failed!");
  Serial.println("Check your EUI's and Keys's!");
  Serial.println("Check if a Gateway is in range!");
}

/**@brief Function for handling LoRaWan received data from Gateway
 *
 * @param[in] app_data  Pointer to rx data
 */
void lorawan_rx_handler(lmh_app_data_t *app_data)
{
  // When we receive a LoRaWAN Packet from the LoRaWAN Gateway, display it.
  // TODO: Ensure that app_data->buffer is null-terminated.
  Serial.printf("LoRa Packet received on port %d, size:%d, rssi:%d, snr:%d, data:%s\n",
          app_data->port, app_data->buffsize, app_data->rssi, app_data->snr, app_data->buffer);
}

// Callback Function that is called when the LoRaWAN Class has been changed
void lorawan_confirm_class_handler(DeviceClass_t Class)
{
  Serial.printf("switch to class %c done\n", "ABC"[Class]);
  // Informs the server that switch has occurred ASAP
  m_lora_app_data.buffsize = 0;
  m_lora_app_data.port = gAppPort;
  lmh_send(&m_lora_app_data, g_CurrentConfirm);
}

// This is called when the 20-second timer expires. We send a LoRaWAN Packet.
void send_lora_frame(void)
{
  if (lmh_join_status_get() != LMH_SET)
  {
    // Not joined, try again later
    return;
  }

  // Copy "Hello!" into the transmit buffer
  m_lora_app_data.port = gAppPort;
  memset(m_lora_app_data.buffer, 0, LORAWAN_APP_DATA_BUFF_SIZE);
  memcpy(m_lora_app_data.buffer, "Hello!", 6);
  m_lora_app_data.buffsize = 6;

  // Transmit the LoRaWAN Packet
  lmh_error_status error = lmh_send(&m_lora_app_data, g_CurrentConfirm);
  if (error == LMH_SUCCESS)
  {
    count++;
    Serial.printf("lmh_send ok count %d\n", count);
  }
  else
  {
    count_fail++;
    Serial.printf("lmh_send fail count %d\n", count_fail);
  }
}

/**@brief Function for handling user timerout event.
 */
void tx_lora_periodic_handler(void)
{
  // When the 20-second timer has expired, send a LoRaWAN Packet and restart the timer
  TimerSetValue(&appTimer, LORAWAN_APP_INTERVAL);
  TimerStart(&appTimer);
  Serial.println("Sending frame now...");
  send_lora_frame();
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
uint32_t timers_init(void)
{
  TimerInit(&appTimer, tx_lora_periodic_handler);
  return 0;
}
