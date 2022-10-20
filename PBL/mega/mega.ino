/*
   Pi Pico CAN id:
    frontLeft(recv 0x011, send 0x021)
    frontRight(recv 0x012, send 0x022)
    rearLeft(recv 0x013, send 0x023)
    rearRight(recv 0x014, send 0x024)

   Mega CAN id:
    recv 0x010, sendAll 0x20?
*/

#include <ArduinoJson.h>
#include <SPI.h>
#include <mcp2515.h>

//JSON doc size
#define JSON_SIZE 512 

//CAN bus variables
static const int MASK = 0x7FF; //CAN module filtering MASK 11bit, 7FF - on all bits
static const int MEGA_ID_RECV = 0x010; //CAN device id for receiving
static const int MEGA_ID_SEND = 0x020; //CAN device id for sending

bool canOk = true; //Is CAN succesfully initiated

//CAN recv/send data frames
can_frame canMsgPicoRecv[4];
can_frame canMsgPicoSend[4];

uint32_t PicoSendIDs[] =  {0x021, 0x022, 0x023, 0x024}; //Pico send IDs in HEX
uint32_t PicoRecvIDs[] =  {0x011, 0x012, 0x013, 0x014}; //Pico recv IDs in HEX

MCP2515 mcp2515(53); //MCP2515 instance, 53-> CS/SS pin

//int counter[2]; //for CAN testing with diodes
String recvStr;

//JSON variables
DynamicJsonDocument recvDoc(JSON_SIZE);
DynamicJsonDocument sendDoc(JSON_SIZE);
String strMotors[] = {"MotorFL", "MotorFR", "MotorRL", "MotorRR"}; //JSON headers

void setup() {
  Serial.begin(115200);

  //CAN module init - true => ok
  canOk = canModuleInit();

  //CAN frames init
  for (int i = 0; i < 4; i++) {
    canMsgPicoRecv[i].can_id = PicoRecvIDs[i];
    canMsgPicoRecv[i].can_dlc = 4;

    canMsgPicoSend[i].can_id = PicoSendIDs[i];
    canMsgPicoSend[i].can_dlc = 4;
  }


  //SENDER
  //pinMode(40, OUTPUT); //for CAN testing with diodes

  //counter[0] = 0; //for CAN testing with diodes
  //counter[1] = 128; //for CAN testing with diodes

  delay(3000); //time for Pi Picos init, maybe not necessary
}

void loop() {
  if (canOk) {
    //SERIAL INSTRUCTIONS READ
    if (Serial.available() > 0) { //Check for data in serial port
      char recvData[JSON_SIZE + 1]; //create char array for received data
      memset(recvData, '\0', JSON_SIZE + 1); //clear array
      unsigned int message_pos = 0; //index of read bytes
      //read data until 2x} (end of json frame)
      while (Serial.available() > 0)
      {
        char inByte = Serial.read();
        if (inByte == '{') {
          recvData[message_pos] = inByte;
          message_pos++;
          while (true) {
            char inByte = Serial.read();
            recvData[message_pos] = inByte;
            if (inByte == '}' && recvData[message_pos - 1] == '}' || inByte == '\n' || inByte == '\r' || inByte == '\0') {
              message_pos++;
              recvData[message_pos] = '\0';
              break;
            }
            else
            {
              recvData[message_pos] = inByte;
              message_pos++;
            }
          }
        }
      }

      //Json deserialize, on success send instructions to Pi Pico via CAN
      DeserializationError error = deserializeJson(recvDoc, buff);
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
      }
      else {
        //CAN instruction send
        for (int i = 0; i < 4; i++) {
          canMsgPicoRecv[i].data[0] = (int) recvDoc[strMotors[i]]["speed"];
          canMsgPicoRecv[i].data[1] = (int) recvDoc[strMotors[i]]["direction"];
          canMsgPicoRecv[i].data[2] = (int) recvDoc[strMotors[i]]["time"];
          canMsgPicoRecv[i].data[2] = 0;

          //char test[200];
          //sprintf(test, "JSON i: %d | %d | %d\n", canMsgPicoRecv[i].data[0], canMsgPicoRecv[i].data[1], canMsgPicoRecv[i].data[2]);
          //Serial.println(test);

          //Try to send data
          if (!canErrorPrint(mcp2515.sendMessage(canMsgPicoRecv + i), "CAN sendMessage: "))
            canOk = false;

          delay(5);
        }
      }
    }

    //CAN DATA RECV
    sendDoc.clear(); //Clear json recv doc
    JsonObject motorsObjects[4]; //JSON nested object for every Pi Pico
    //Recv data from pi pico
    for (int i = 0; i < 4; i++) {
      if (mcp2515.readMessage(&canMsgPicoRecv[i]), "CAN readMessage: ") {
        JsonObject motorsObjects[i] = sendDoc.createNestedObject(strMotors[i]);

        //assign received data to JSON object varaibles, data type may be different
        motorsObjects[i]["ValA"] = (int) canMsgPicoRecv[i].data[0];
        motorsObjects[i]["ValB"] = (int) canMsgPicoRecv[i].data[1];
        motorsObjects[i]["ValC"] = (int) canMsgPicoRecv[i].data[2];
        motorsObjects[i]["ValD"] = (int) canMsgPicoRecv[i].data[3];
      }
      delay(1);
    }
    serializeJson(sendDoc, Serial); //Send received data via Serail Port
  }
  else {
    //if CAN module failed, wait 5sec and try to init
    delay(5000);
    canOk = canModuleInit();
  }

  //for CAN testing with diodes
  /*
    digitalWrite(40, HIGH);
    for (int i = 0; i < 2; i++)
    if (counter[i] > 255)
      counter[i] = 0;

    for (int i = 0; i < 4; i++) {
    if (i % 2 == 0) {
      canMsgPicoRecv[i].data[0] = 255 - counter[0];
      canMsgPicoRecv[i].data[1] = (int) counter[0] * 0.5;
      canMsgPicoRecv[i].data[2] = 0x00;
      canMsgPicoRecv[i].data[3] = 0x00;
    }
    else {
      canMsgPicoRecv[i].data[0] = counter[0];
      canMsgPicoRecv[i].data[1] = (int) (255 - counter[1]) * 0.2;
      canMsgPicoRecv[i].data[2] = 0x00;
      canMsgPicoRecv[i].data[3] = 0x00;
    }
    }

    for (int i = 0; i < 4; i++) {
    if (!canErrorPrint(mcp2515.sendMessage(canMsgPicoRecv + i), "CAN sendMessage: "))
      canOk = false;
    else {
      //Serial.print("Send data [%d]: %d | %d | %d | %d\n", i, canMsgPicoRecv[i].data[0], canMsgPicoRecv[i].data[1], canMsgPicoRecv[i].data[2], canMsgPicoRecv[i].data[3]);
      Serial.print(i);
      Serial.print(") Send!\n");
    }
    delay(5);
    }

    Serial.print("\n");

    for (int i = 0; i < 2; i++)
    counter[i] += 30;

    digitalWrite(40, LOW);
  */
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

  //For MASK0, two filters
  if (!canErrorPrint(mcp2515.setFilterMask(MCP2515::MASK0, false, MASK), "Can setFilterMask0: "))
    return false;
  if (!canErrorPrint(mcp2515.setFilter(MCP2515::RXF0, false, 0x021), "Can setFilter_RXF0: "))
    return false;
  if (!canErrorPrint(mcp2515.setFilter(MCP2515::RXF1, false, 0x022), "Can setFilter_RXF1: "))
    return false;

  //For MASK1, three filters
  if (!canErrorPrint(mcp2515.setFilterMask(MCP2515::MASK1, false, MASK), "Can setFilterMask1: "))
    return false;
  if (!canErrorPrint(mcp2515.setFilter(MCP2515::RXF2, false, 0x023), "Can setFilter_RXF2: "))
    return false;
  if (!canErrorPrint(mcp2515.setFilter(MCP2515::RXF3, false, 0x024), "Can setFilter_RXF3: "))
    return false;
  if (!canErrorPrint(mcp2515.setFilter(MCP2515::RXF4, false, 0x021), "Can setFilter_RXF4: ")) //not used, same as RXF0
    return false;

  //Setting normal mode after filtering setup
  if (!canErrorPrint(mcp2515.setNormalMode(), "Can setNormalMode: "))
    return false;

  return true;
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
