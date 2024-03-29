//BottleBoats - HMS F(l)ounder

#define GPSRX 10 //Constant for GPS Receiver Pin
#define GPSTX 11 //Constant for GPS Transmitter Pin
#define MOTOR 5  //Constant for Motor Control Pin
#define SERVO 6  //Constant for Servo Control Pin
#define address 0x1E //0011110b, I2C 7bit address of HMC5883

#include <Wire.h> //I2C Arduino Library
#include <math.h>
#include <Servo.h>
#include <SoftwareSerial.h> //SoftwareSerial Lib
SoftwareSerial GPSSerial(GPSRX, GPSTX); // RX, TX
Servo Rudder;

const char RudderDegs[6] = {0,9,18,27,36,45};

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
  Rudder.attach(SERVO); //Set up Rudder Servo
  pinMode(MOTOR, OUTPUT); //Set up Motor output pin
}

void loop() {
  String RawRMC = GPSRMC();     //Receive a string of the latest GPS Data
  float Bearing;              //Create the GPSNorth Variable - for storing the real North Coordinates
  char Direction;
  //String GPSWest;               //Create the GPSWest Variable - for storing the real West Coordinates

  if (! RawRMC.equals("!RMC")) {      //Check that the received data is GPRMC as we want it
    Serial.println("Heading: ");
    Serial.println(CompReceive());
    Serial.println(RawRMC);

    if (GPSReady(RawRMC) == true) {   //Check if the RMC data contains a valid GPS Fix
      Bearing = CalcBearing(RawRMC);   //Set GPSNorth to the variable returned from the function which calculates the North coordinates from the RMC
      //Serial.println(Bearing);
      RudderCont(Bearing, Direction);
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

float CalcBearing(String GPRMC) {
  char c;
  float Bearing, latitude, longitude; //Numeric values for bearing, latitude and longitude
  float LatArcMin, LongArcMin; //Holds the arc minutes for latitude and longitude
  String LatRawString = "", LongRawString = ""; //Holds the raw data from GPRMC for the latitude and longitude
  String LatString = "", LongString = ""; //Holds the degrees lat / long in string format to be changed
  int j = 0;

  for (int i = 0; i < GPRMC.length(); i++) {  //Loop through the entire GPRMC String
    c = GPRMC.charAt(i);  //Set c to the character at that point in the GPRMC String
    if (c == ',') {       //Check whether the loop has arrived at a seperating comma
      j++;                //Increment j counter
    }

    if (j == 3) {            //Check if the loop has reached the comma before the GPS Latitude
      int k = i;
      while (GPRMC.charAt(k) != ',') {
        LatRawString += GPRMC.charAt(k); //Add character at point k to the entire latitude string
      }
    }

    if (j == 5) {            //Check if the loop has reached the comma before the GPS Longitude
      int k = i;
      while (GPRMC.charAt(k) != ',') {
        LongRawString += GPRMC.charAt(k); //Add character at point k to the entire longitude string
      }
      break;              //Escape loop and go on to bearing calculation
    }
  }

  //Converting the Raw data into Degrees and arcMinutes - Latitude
  for (int i = 0 ; i < 2 ; i ++) { //Loops twice, as the first 2 characters are the degrees according to the standard
    LatString += LatRawString.charAt(i);
    LatRawString.setCharAt(i, '0'); //Sets the character we just read to 0, so it is parsable later on for ArcMinutes
  }
  
  latitude = LatString.toFloat();     //Converts the characters we just pulled into a float
  LatArcMin = LatRawString.toFloat(); //Converts the rest of the characters, including the 0's we added into a float

  //Converting the Raw data into Degrees and arcMinutes - Longitude
  for (int i = 0 ; i < 3 ; i++) { //Loops thrice, as the first 3 characters are the degrees according to the standard
    LongString += LongRawString.charAt(i);
    LatRawString.setCharAt(i, '0'); //Sets the character we just read to 0, so it is parsable later on for ArcMinutes
  }
  longitude = LongString.toFloat();     //Converts the characters we just pulled into a float
  LongArcMin = LongRawString.toFloat(); //Converts the rest of the characters, including the 0's we added into a float

  
  Serial.println(latitude); //Temporary debugging
  Serial.println(longitude); //Temporary debugging

  return (Bearing); //Return Bearing Value
}

/*
   Converts the given arc minutes in the GPRMC into degrees. Works for both
   longitude and latitude. It should be noted that due to arduino restrictions,
   we are only able to have floats with 6-7 decimals of accuracy.

   param float arcMinutes
      The arcminutes given by the GPSRMC to be converted

   return float degrees
      The converted arcminutes is returned in degrees
*/
float ArcToDeg(float ArcMinutes) {
  float degrees;

  degrees = ArcMinutes / 60; //Dividing by 60, as there are 60 arcMinutes for the earth
  Serial.println(degrees, 5); //Println for testing purposes, prints up to 4 decimal places.

  return degrees;
}

/*
   Takes the current latitude and longitude supplied by the GPRMC and
   uses it with the checkpoint data to find the bearing needed to reach that
   destination.

   param float latitude
      The current latitude from the GPRMC

   param float longitude
      The current longitude from the GPRMC

   returns float bearing
      The calculated bearing needed to reach the destination co-ordinates
*/
float DestinationBearing(float latitude, float longitude) {
  float bearing, x, y;
  float DestLat, DestLong; //Temporary destinations - TaMed benches

  DestLat = 52.41769; //Temporary destination Latitude - TaMed benches
  DestLong = -4.06487; //Temporary destination Longitude - TaMed benches

  x = cos(DestLat) * sin(DestLong - longitude);

  y = (cos(latitude) * sin(DestLat)) - (sin(latitude) * cos(DestLat) * cos(DestLong - longitude));

  bearing = atan2(x, y);

  return bearing;
}

void RudderCont(float Bear, char Dir) {
char RudderPos = 0; //A Position from 0 - 5
  if (Bear <36) {
    RudderPos = 1; //Check the bearing between the boat and the waypoint and set the position to position 1 if it's a small difference
    } else if (Bear <72) {
      RudderPos = 2; //Check the bearing between the boat and the waypoint and set the position to position 2 if it's a slightly larger difference
      } else if (Bear <108) {
        RudderPos = 3; //Check the bearing between the boat and the waypoint and set the position to position 3 if it's a medium difference
        } else if (Bear <144) {
          RudderPos = 4; //Check the bearing between the boat and the waypoint and set the position to position 4 if it's a large difference
          } else if (Bear <=180) {
            RudderPos = 5; //Check the bearing between the boat and the waypoint and set the position to position 5 if it's a massive difference
          }
  Rudder.write(RudderDegs[RudderPos]);  //Write the Rudder Position value (in an array of degrees positions for the servo) to the Servo
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
            68          mandatory checksum



  Real GPRMC Example from Penbryn - if Penbryn was in Russia (Paul)

  $GPRMC,125914.00,A,5225.08700,N,00403.86768,W,0.325,,181115,,,A*6E
*/
