//BottleBoats - HMS F(l)ounder

#define GPSRX 10 //Constant for GPS Receiver Pin
#define GPSTX 11 //Constant for GPS Transmitter Pin
#define address 0x1E //0011110b, I2C 7bit address of HMC5883

#include <Wire.h> //I2C Arduino Library
#include <math.h>
#include <SoftwareSerial.h> //SoftwareSerial Lib
SoftwareSerial GPSSerial(GPSRX, GPSTX); // RX, TX

void setup() {
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

void loop() {
  String RawRMC = GPSRMC();     //Receive a string of the latest GPS Data
  String Bearing;              //Create the GPSNorth Variable - for storing the real North Coordinates
  //String GPSWest;               //Create the GPSWest Variable - for storing the real West Coordinates

  if (! RawRMC.equals("!RMC")) {      //Check that the received data is GPRMC as we want it
    Serial.println("Heading: ");
    Serial.println(CompReceive());
    Serial.println(RawRMC);

    if (GPSReady(RawRMC) == true) {   //Check if the RMC data contains a valid GPS Fix
      Bearing = CalcBearing(RawRMC);   //Set GPSNorth to the variable returned from the function which calculates the North coordinates from the RMC
    }
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

  if (CheckStr.equals("RMC")) {         //Check that we have the GPS message we want
    ReturnStr = GPSStr;                 //If it's the correct message, return it
  } else {
    ReturnStr = "!RMC";                 //Return a message which is interpreted so as not to be used
  }

  return (ReturnStr);                   //Return the string to where it was called
}

double CompReceive() {
  static int x, y, z; //triple axis data

  //Tell the HMC5883 where to begin reading data
  Wire.beginTransmission(address);
  Wire.write(0x03); //select register 3, X MSB register
  Wire.endTransmission();

  //Read data from each axis, 2 registers per axis
  Wire.requestFrom(address, 6);
  if (6 <= Wire.available()) {
    x = Wire.read() << 8; //X msb
    x |= Wire.read(); //X lsb
    z = Wire.read() << 8; //Z msb
    z |= Wire.read(); //Z lsb
    y = Wire.read() << 8; //Y msb
    y |= Wire.read(); //Y lsb
  }

  float heading = atan2(y, x);
  if (heading < 0)
    heading += 2 * M_PI;
  return (heading * 180 / M_PI);

  delay(250);
}

boolean GPSReady(String GPRMC) {
  char c;
  char StatChar;
  int j = 0;
  boolean Status;

  for (int i = 0; i < GPRMC.length(); i++) {  //Loop through the entire GPRMC String
    c = GPRMC.charAt(i);  //Set c to the character at that point in the GPRMC String
    if (c == ',') {       //Check whether the loop has arrived at a seperating comma
      j++;                //Increment j counter
    }

    if (j == 2) {            //Check if the loop has reached the comma before the GPS Status
      StatChar = GPRMC.charAt(i + 1); //Set character variable to the status character
      break;              //Escape loop and go on to return
    }
  }

  if (StatChar == 'V') {   //V means GPS value warning - no GPS
    Status = false;       //Return that there are no valid GPS coordinates to use
  } else {
    Status = true;        //Return that there are valid GPS coordinates to use
  }
  return Status;          //Return the status of the GPS Coordinates
}

String CalcBearing(String GPRMC) {
  char c;
  String Lat = "";
  String Long = "";
  int j = 0;
  boolean Status;

  for (int i = 0; i < GPRMC.length(); i++) {  //Loop through the entire GPRMC String
    c = GPRMC.charAt(i);  //Set c to the character at that point in the GPRMC String
    if (c == ',') {       //Check whether the loop has arrived at a seperating comma
      j++;                //Increment j counter
    }

    if (j == 3) {            //Check if the loop has reached the comma before the GPS Latitude
      int k = i;
      while (GPRMC.charAt(k) != ',') {
        Lat += GPRMC.charAt(k); //Set character variable to the status character
      }
    }

    if (j == 5) {            //Check if the loop has reached the comma before the GPS Longitude
      int k = i;
      while (GPRMC.charAt(k) != ',') {
        Long = GPRMC.charAt(k); //Set character variable to the status character
      }
      break;              //Escape loop and go on to return
    }
  }

  if (StatChar == 'V') {   //V means GPS value warning - no GPS
    Status = false;       //Return that there are no valid GPS coordinates to use
  } else {
    Status = true;        //Return that there are valid GPS coordinates to use
  }
  return Status;          //Return the status of the GPS Coordinates
}

/*
 Example GPRMC
 $GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191194,020.3,E*68


           225446       Time of fix 22:54:46 UTC
           A            Navigation receiver warning A = OK, V = warning
           4916.45,N    Latitude 49 deg. 16.45 min North
           12311.12,W   Longitude 123 deg. 11.12 min West
           000.5        Speed over ground, Knots
           054.7        Course Made Good, True
           191194       Date of fix  19 November 1994
           020.3,E      Magnetic variation 20.3 deg East
           *68          mandatory checksum



  Real GPRMC Example from Penbryn

  $GPRMC,125914.00,A,5225.08700,N,00403.86768,W,0.325,,181115,,,A*6E
*/
