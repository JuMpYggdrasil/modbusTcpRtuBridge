#include <ESP8266WiFi.h>
#include <ModbusIP_ESP8266.h>//https://github.com/andresarmento/modbus-esp8266
#include <ModbusRTU.h>

//#define USE_STATIC_IP

// COM    <=>    ESP8266    <=>    MEGA2560
//        WIFI              UART
// [MODBUS]|                 |                 |
// TCP     | TCP    .    RTU | RTU       SENSOR|
// MASTER  | SLAVE  . MASTER | SLAVE           |
// client  | server .        |                 |

//*note:loop handle get value from RTU SLAVE to TCP SLAVE. and wait TCP MASTER.
//      callback handle set value from TCP MASTER to RTU SLAVE.
// help https://www.modbustools.com/modbus.html#Function01

#define TO_REG 0   //address map to ModbusIP (can map address with offset)
#define SLAVE_ID 1 //ID of ModbusIP
#define PULL_ID 1  //ID of ModbusRTU
#define FROM_REG 0 //address map from ModbusRTU
#define n_LEN_Hreg 30   //size of address table holding register
#define n_LEN_Ireg 30   //size of address table input register
#define n_LEN_Coil 14   //size of address table coil

#ifdef USE_STATIC_IP
IPAddress local_IP(192, 168, 1, 41);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
#endif

const char* ssid = "JUMP";
const char* passssid = "025260652";

ModbusRTU mbRTU;
ModbusIP mbTCP;

//uint16_t callbackGetCoil(TRegister* reg, uint16_t val) {
//  bool res;
//  if ( !mbRTU.slave() ) {
//    mbRTU.readCoil(SLAVE_ID, reg->address.address, &res);
//    while( mbRTU.slave() ) {
//      mbRTU.task();
//    }
//  }
//  return val;
//}

uint16_t callbackSetCoil(TRegister* reg, uint16_t val) {
    if ( !mbRTU.slave() ) {
        uint16_t res = {val};
        mbRTU.writeCoil(SLAVE_ID, reg->address.address, COIL_BOOL(res));
        while ( mbRTU.slave() ) {
            mbRTU.task();
        }
    }
    return val;
}
//uint16_t callbackGetHreg(TRegister* reg, uint16_t val) {
//  uint16_t res[1];
//  if ( !mbRTU.slave() ) {
//    mbRTU.readHreg(SLAVE_ID, reg->address.address, res);
//    while( mbRTU.slave() ) {
//      mbRTU.task();
//    }
//  }
//  return res[0];
//}

uint16_t callbackSetHreg(TRegister* reg, uint16_t val) {
    if ( !mbRTU.slave() ) {
        uint16_t res = {val};
        mbRTU.writeHreg(SLAVE_ID, reg->address.address, res);
        while ( mbRTU.slave() ) {
            mbRTU.task();
        }
    }
    return val;
}

void setup() {
    Serial.begin(9600, SERIAL_8N1);

#ifdef USE_STATIC_IP
    WiFi.config(local_IP, primaryDNS, gateway, subnet);
#endif
    WiFi.begin(ssid, passssid);//MAC 2C:F4:32:4C:C2:E8
    WiFi.hostname("EGAT");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    //    Serial.println("");
    //    Serial.println("WiFi connected");
    //    Serial.println("IP address: ");
    //    Serial.println(WiFi.localIP());

    //Serial1.begin(9600, SERIAL_8N1); // Init Serial on default pins
    //Serial2.begin(19200, SERIAL_8N1, 19, 18); // Override default pins for ESP32
    mbRTU.begin(&Serial);
    //mbRTU.begin(&Serial2, 17);  // Specify RE_DE control pin
    mbRTU.master();
    mbTCP.server();
    mbTCP.addHreg(0, 0, n_LEN_Hreg);//start address, data value, n of length of address
    mbTCP.addIreg(0, 0, n_LEN_Ireg);
    mbTCP.addCoil(0, COIL_VAL(true), n_LEN_Coil);
    //callback fn
    //mbTCP.onGetCoil(0, callbackGetCoil, n_LEN_Coil);
    mbTCP.onSetCoil(0, callbackSetCoil, n_LEN_Coil);
    //mbTCP.onGetHreg(0, callbackGetHreg, n_LEN_Hreg);
    mbTCP.onSetHreg(0, callbackSetHreg, n_LEN_Hreg);
}

void loop() {
    if (!mbRTU.slave()) {
        mbRTU.pullHreg(PULL_ID, FROM_REG, TO_REG, n_LEN_Hreg);
        while (mbRTU.slave()) {
            mbRTU.task();
            delay(50);
        }
    }
    mbRTU.task();
    mbTCP.task();
    if (!mbRTU.slave()) {
        mbRTU.pullCoil(PULL_ID, FROM_REG, TO_REG, n_LEN_Coil);
        //mbRTU.readCoil(1, 1, coils, 20, cbWrite);
        while (mbRTU.slave()) {
            mbRTU.task();
            delay(50);
        }
    }
    mbRTU.task();
    mbTCP.task();
    if (!mbRTU.slave()) {
        mbRTU.pullIreg(PULL_ID, FROM_REG, TO_REG, n_LEN_Ireg);
        while (mbRTU.slave()) {
            mbRTU.task();
            delay(50);
        }
    }
    mbRTU.task();
    mbTCP.task();
    yield();
}
