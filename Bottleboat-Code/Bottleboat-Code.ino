//BottleBoats - HMS F(l)ounder

#define GPSRX 10 //Constant for GPS Receiver Pin
#define GPSTX 11 //Constant for GPS Transmitter Pin
#define address 0x1E //0011110b, I2C 7bit address of HMC5883

#include <Wire.h> //I2C Arduino Library
#include <math.h>
#include <SoftwareSerial.h> //SoftwareSerial Lib
SoftwareSerial GPSSerial(GPSRX, GPSTX); // RX, TX

void setup(){
  //Initialize Serial and I2C communications
  Serial.begin(57600);
  GPSSerial.begin(9600); //Set the rate for the GPS Serial Port

  Wire.begin();
  //Put the HMC5883 IC into the correct operating mode
  Wire.beginTransmission(address); //open communication with HMC5883
  Wire.write(0x02); //select mode register
  Wire.write(0x00); //continuous measurement mode
  Wire.endTransmission();
}

void loop(){
  String RawRMC = GPSRMC();
  
    if (! RawRMC.equals("!RMC")) {
      Serial.println("Heading: ");
      Serial.println(CompReceive());
      Serial.println(RawRMC);
    }
}

String GPSRMC() {
  String GPSStr = "";
  bool flag = false;
  int Count = 0;
  String CheckStr = "";
  String ReturnStr = "";
  
  while (flag == false) {
    while (! GPSSerial.available()) {}  //Loops to wait until GPS Serial is ready to send
    char c = GPSSerial.read();          //Set char c to the sent character
    
    if (c != '$') {                 //Check that it is not a new set of data starting with $
        GPSStr += c;                //Append the character to the string
    } else {
        flag = true;                //Set the flag to true if a new set of data starts sending to break the loop
    }
  }

      for (int j = 2; j < 5; j++) {         //Loop from the position of $ until the end of the message ID
        CheckStr += GPSStr.charAt(j);       //Add each character of the message to another string
      }
      
      if (CheckStr.equals("RMC")){          //Check that we have the GPS message we want
        ReturnStr = GPSStr;                 //If it's the correct message, return it
      } else {
        ReturnStr = "!RMC";                 //Return a message which is interpreted so as not to be used
      }

      return(ReturnStr);                    //Return the string to where it was called
}

double CompReceive() {
  static int x,y,z; //triple axis data

  //Tell the HMC5883 where to begin reading data
  Wire.beginTransmission(address);
  Wire.write(0x03); //select register 3, X MSB register
  Wire.endTransmission();
  
 //Read data from each axis, 2 registers per axis
  Wire.requestFrom(address, 6);
  if(6<=Wire.available()){
    x = Wire.read()<<8; //X msb
    x |= Wire.read(); //X lsb
    z = Wire.read()<<8; //Z msb
    z |= Wire.read(); //Z lsb
    y = Wire.read()<<8; //Y msb
    y |= Wire.read(); //Y lsb
  }

    float heading = atan2(y, x);
    if(heading < 0)
      heading += 2 * M_PI;
    return(heading * 180/M_PI);

    delay(250);
}

