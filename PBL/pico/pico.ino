/*
   Pi Pico CAN id:
    frontLeft(recv 0x011, send 0x021)
    frontRight(recv 0x012, send 0x022)
    rearLeft(recv 0x013, send 0x023)
    rearRight(recv 0x014, send 0x024)

   Mega CAN id:
    recv 0x010, sendAll 0x20
*/

#include <SPI.h>
#include "mcp2515.h"

//CAN bus variables
static const int MASK = 0x7FF; //CAN module filtering MASK 11bit, 7FF - on all bits
static const int PICO_ID_RECV = 0x014; //CAN device id for receiving
static const int PICO_ID_SEND = 0x024; //CAN device id for sending

bool canOk = true; //Is CAN succesfully initiated

//Assigning pins for MCP2515 module
static const int PICO_SCK = 2;
static const int PICO_MOSI = 3;
static const int PICO_MISO = 4;
static const int PICO_CS = 5;

//Assigning phase detection pins
static const int DETECTION_L1 = 14;
static const int DETECTION_L2 = 15;
static const int DETECTION_L3 = 16;

//Phase passing through 0
int zeroPhase = 0;

//creating mcp2515 object, assigning SPI channel, pins and frequency
MCP2515 mcp2515(spi0, PICO_CS, PICO_MOSI, PICO_MISO, PICO_SCK, 10000000);

struct can_frame canMsgRecv, canMsgSend; //CAN data frames send/recv

void setup() {
  Serial.begin(115200);
  Serial.println();

  //assigning SPI pins
  SPI.setSCK(PICO_SCK);   // SCK
  SPI.setTX(PICO_MOSI);   // MOSI
  SPI.setRX(PICO_MISO);   // MISO
  SPI.setCS(PICO_CS);     // CS

  pinMode(LED_BUILTIN, OUTPUT);

  //setting high level on 23 pin for more accurate ADC readings
  pinMode(23, OUTPUT);
  digitalWrite(23, HIGH);

  delay(200); //delay before CAN init

  //CAN module init - true => success
  canOk = canModuleInit();

  //RECEIVER
  //pinMode(10, OUTPUT); //for CAN testing with diodes
  //pinMode(11, OUTPUT); //for CAN testing with diodes


  digitalWrite(LED_BUILTIN, HIGH); //Build in diode on after setup
}

void loop() {
  if (canOk) {
    //CAN recv
    if (canErrorPrint(mcp2515.readMessage(&canMsgRecv), "CAN readMessage: ")) {
      int d0 = canMsgRecv.data[0];
      int d1 = canMsgRecv.data[1];
      int d2 = canMsgRecv.data[2];
      int d3 = canMsgRecv.data[3];
      Serial.printf("Recv data [%d]: %d | %d | %d | %d\n", canMsgRecv.can_id, d0, d1, d2, d3);
      //analogWrite(10, d0); //for CAN testing with diodes
      //analogWrite(11, d1); //for CAN testing with diodes
    }

    //CAN send
    canMsgSend.can_id = PICO_ID_SEND;
    canMsgSend.can_dlc = 4;
    //assigning values to CAN frame data fields
    canMsgSend.data[0] = 0x00; //e.g. analogRead(26)
    canMsgSend.data[1] = 0x00;
    canMsgSend.data[2] = 0x00;
    canMsgSend.data[3] = 0x00;

    //send data via CAN
    if (canErrorPrint(mcp2515.sendMessage(&canMsgSend), "CAN sendMessage: "))
      Serial.printf("Send data: %d | %d | %d | %d\n", canMsgSend.data[0], canMsgSend.data[1], canMsgSend.data[2], canMsgSend.data[3]);

  }
  else {
    //if CAN module failed, wait 5sec and try to init
    delay(5000);
    canOk = canModuleInit();
  }

  delay(1);
}

//CAN communication and filtering setup, returns true on success
bool canModuleInit() {
  //CAN communication config (bitrate and filtering)
  if (!canErrorPrint(mcp2515.reset(), "Can reset: "))
    return false;
  if (!canErrorPrint(mcp2515.setBitrate(CAN_80KBPS, MCP_16MHZ), "Can setBitrate: "))
    return false;

  //Setting CAN filtering for data receiving in config mode
  canErrorPrint(mcp2515.setConfigMode(), "Can setConfigMode: ");

  //Allow instructions 'adressed' only to this device

  //For MASK0, two filters
  if (!canErrorPrint(mcp2515.setFilterMask(MCP2515::MASK0, false, MASK), "Can setFilterMask0: "))
    return false;
  if (!canErrorPrint(mcp2515.setFilter(MCP2515::RXF0, false, PICO_ID_RECV), "Can setFilter_RXF0: "))
    return false;
  if (!canErrorPrint(mcp2515.setFilter(MCP2515::RXF1, false, PICO_ID_RECV), "Can setFilter_RXF1: "))
    return false;

  //For MASK1, three filters
  if (!canErrorPrint(mcp2515.setFilterMask(MCP2515::MASK1, false, MASK), "Can setFilterMask1: "))
    return false;
  if (!canErrorPrint(mcp2515.setFilter(MCP2515::RXF2, false, PICO_ID_RECV), "Can setFilter_RXF2: "))
    return false;
  if (!canErrorPrint(mcp2515.setFilter(MCP2515::RXF3, false, PICO_ID_RECV), "Can setFilter_RXF3: "))
    return false;
  if (!canErrorPrint(mcp2515.setFilter(MCP2515::RXF4, false, PICO_ID_RECV), "Can setFilter_RXF4: "))
    return false;

  //Setting normal mode after filtering setup
  if (!canErrorPrint(mcp2515.setNormalMode(), "Can setNormalMode: "))
    return false;

  return true;
}

//Functions initiates interrupts on pins connected to analog comparators
void initInterrupts() {
  pinMode(DETECTION_L1, INPUT);
  pinMode(DETECTION_L1, INPUT);
  pinMode(DETECTION_L1, INPUT);

  attachInterrupt(DETECTION_L1, changedL1, CHANGE);
  attachInterrupt(DETECTION_L2, changedL2, CHANGE);
  attachInterrupt(DETECTION_L3, changedL3, CHANGE);
}

//Interrupt handling L1
void changedL1() {
  zeroPhase = 1;
}

//Interrupt handling L2
void changedL2() {
  zeroPhase = 2;
}

//Interrupt handling L3
void changedL3() {
  zeroPhase = 3;
}

//CAN error code check and print, returns true if error = OK, else false
bool canErrorPrint(int error, const char* text) {
  Serial.print(text);

  switch (error) {
    case MCP2515::ERROR_OK:
      Serial.print("Ok\n");
      return true;
      break;

    case MCP2515::ERROR_FAIL:
      Serial.print("Fail\n");
      break;

    case MCP2515::ERROR_ALLTXBUSY:
      Serial.print("All TX busy\n");
      break;
    case MCP2515::ERROR_FAILINIT:
      Serial.print("Fail Init\n");
      break;

    case MCP2515::ERROR_FAILTX:
      Serial.print("Fail TX\n");
      break;

    case MCP2515::ERROR_NOMSG:
      Serial.print("No message\n");
      break;

    default:
      Serial.print("Inny blad\n");
      break;
  }
  return false;
}
