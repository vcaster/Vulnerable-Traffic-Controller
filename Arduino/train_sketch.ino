#include <SPI.h>
#include <Ethernet.h>

#include <ArduinoRS485.h>  // ArduinoModbus depends on the ArduinoRS485 library
#include <ArduinoModbus.h>

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(10, 10, 30, 10); // assignes the arduino an IP address

int eastStop1;
int eastStop2;
int northStop1;
int northStop2;

int holdReg;
int Speed = 255;

EthernetClient ethClient;
ModbusTCPClient modbusTCPClient(ethClient);

IPAddress server(10, 10, 30, 2);  // IP Address of your Modbus server
void start() {
  // Starts all movement which sends the signal to the motor driver
  digitalWrite(2, HIGH);
  digitalWrite(13, LOW);
  digitalWrite(7, LOW);
  digitalWrite(8, HIGH);
}
void stopEastT() {
  //Stops east traffic which sends the signal to the motor driver
  digitalWrite(7, LOW);
  digitalWrite(8, LOW);
}

void stopNorthT() {
  //Stosp north traffic which sends the signal to the motor driver
  digitalWrite(2, LOW);
  digitalWrite(13, LOW);
}
void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  // while (!Serial) {
  //   ; // wait for serial port to connect. Needed for native USB port only
  // }

  // disable SD SPI
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);


  pinMode(3, OUTPUT); // declare pin 3 used by the motor driver to set the speed
  pinMode(11, OUTPUT); // declare pin 11 used by the motor driver to set the speed
  pinMode(2, OUTPUT); // declare pin 2 used by the motor driver to power the North traffic
  pinMode(13, OUTPUT); // declare pin 13 used by the motor driver to power the North traffic
  pinMode(7, OUTPUT); // declare pin 7 used by the motor driver to power the East traffic
  pinMode(8, OUTPUT); // declare pin 7 used by the motor driver to power the East traffic

  // set speed
  analogWrite(3, Speed);
  analogWrite(11, Speed);

  start();

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  Serial.println(Ethernet.hardwareStatus());
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1);  // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }
}

void loop() {
  if (!modbusTCPClient.connected()) {
    // client not connected, start the Modbus TCP client
    Serial.println("Attempting to connect to Modbus TCP server");

    if (!modbusTCPClient.begin(server, 503)) {
      Serial.println("Modbus TCP Client failed to connect!");
    } else {
      //Serial.println("Modbus TCP Client connected");
    }
  } else {
    // client connected

    // read the value 0x00 == 40001

    if (!modbusTCPClient.holdingRegisterRead(0x00, 0x00)) {
      //Serial.println("Register returned 0 \n Stop all movement");
      //Serial.println(modbusTCPClient.lastError());

      start();

      // delay(10000);
    } else {
      //Serial.print("Holding Register");
      holdReg = modbusTCPClient.holdingRegisterRead(0x00);
      // Serial.println(holdReg); //prints the value of the holding registers in decimal

      // read the infraed sensors // any value less than 500 means the train is approaching an intersection
      eastStop1 = analogRead(A0);
      eastStop2 = analogRead(A1);
      northStop1 = analogRead(A2);
      northStop2 = analogRead(A3);

    // holdReg == 8960 means the traffic signal is RED, RED
      while (eastStop1 < 500 || eastStop2 < 500 || northStop1 < 500 || northStop2 < 500) {
        // read the infraed sensors
        eastStop1 = analogRead(A0);
        eastStop2 = analogRead(A1);
        northStop1 = analogRead(A2);
        northStop2 = analogRead(A3);
        if (holdReg == 8960 && (eastStop1 < 500 || eastStop2 < 500) && (northStop1 < 500 || northStop2 < 500)) {
          stopEastT();
          stopNorthT();
          holdReg = modbusTCPClient.holdingRegisterRead(0x00);
        }
        else if (holdReg == 8960 && (eastStop1 < 500 || eastStop2 < 500)) {
          stopEastT();
          stopNorthT();
          holdReg = modbusTCPClient.holdingRegisterRead(0x00);
        }
        else if (holdReg == 8960 && (northStop1 < 500 || northStop2 < 500)) {
          stopNorthT();
          holdReg = modbusTCPClient.holdingRegisterRead(0x00);
        } else {
          break;
        }
      }

      // Serial.println(eastStop1);

      // East
      // holdReg == -32000 means the traffic signal is RED, YELLOW
      // holdReg == 17152 means the traffic signal is RED, GREEN
      while (eastStop1 < 500) {
        eastStop1 = analogRead(A0);
        if (holdReg == -32000 || holdReg == 17152) {
          // Serial.println("stopped east1");
          stopEastT();
          holdReg = modbusTCPClient.holdingRegisterRead(0x00);
        } else {
          break;
        }
      }

      holdReg = modbusTCPClient.holdingRegisterRead(0x00);
      // read the infraed sensors
      eastStop1 = analogRead(A0);
      eastStop2 = analogRead(A1);
      northStop1 = analogRead(A2);
      northStop2 = analogRead(A3);

      while (eastStop2 < 500) {
        eastStop2 = analogRead(A1);
        if (holdReg == -32000 || holdReg == 17152) {
          // Serial.println("stopped east2");
          stopEastT();
          holdReg = modbusTCPClient.holdingRegisterRead(0x00);
        } else {
          break;
        }
      }

      //North
      // holdReg == 9472 means the traffic signal is GREEN, RED
      // holdReg == 10496 means the traffic signal is YELLOW, RED
      holdReg = modbusTCPClient.holdingRegisterRead(0x00);
      northStop1 = analogRead(A2);
      northStop2 = analogRead(A3);

      while (northStop1 < 500) {
        northStop1 = analogRead(A2);
        if (holdReg == 9472 || holdReg == 10496) {
          // Serial.println("stopped north1");
          stopNorthT();
          holdReg = modbusTCPClient.holdingRegisterRead(0x00);
        } else {
          break;
        }
      }

      holdReg = modbusTCPClient.holdingRegisterRead(0x00);
      northStop1 = analogRead(A2);
      northStop2 = analogRead(A3);

      while (northStop2 < 500) {
        northStop2 = analogRead(A3);
        if (holdReg == 9472 || holdReg == 10496) {
          // Serial.println("stopped north2");
          stopNorthT();
          holdReg = modbusTCPClient.holdingRegisterRead(0x00);
        } else {
          break;
        }
      }
    }

    // wait for 1 second
    start();
    // delay(500);
  }
}