/*
  =============================================================
  Program for Pi Pico and SIM7000
  7/21/2023
  =============================================================
*/


#define SIMCOM_KEY 2  // set Pico pin for KEY pin to turn SIMCOM chip on and off
                      // this is GP2, or pin #4, on Pi Pico
                      // note: not all cell boards have a KEY pin

// for blink_LED()
unsigned long previousMillis = 0;
unsigned long currentMillis = 0;
const long interval_ON = 25;
const long interval_OFF = 3000;
int ledState = LOW;

char StringOK[] = "OK";
char StringGreaterThanSign[] = "> ";

char command;

String SIMCOM_String = "";  // serial from SIMCOM
String AT_String = "";      // serial to SIMCOM
String SMS_String = "";     // text message string

String Cell_Num = "14405395007";

String GPS_ON = "";
String GPS_Fix = "";
String GPS_Time = "";
String GPS_Lat = "38.889484";     //  initial is Lat and Long
String GPS_Long = "-77.035278";   //  of Washington Monument
String GPS_Alt = "";
String LatLongURL = "";


// START SETUP **************************************************
void setup() {

  // define pins
  pinMode(LED_BUILTIN, OUTPUT);     // set LED
  pinMode(SIMCOM_KEY, OUTPUT);      // set Pico pin for SIMCOM KEY pin
  digitalWrite(SIMCOM_KEY, HIGH);   // start high

  // define serial
  Serial.begin(9600);
  Serial1.setFIFOSize(128);   // otherwise serial1 buffer size is 32 bytes by default, too small
  Serial1.begin(9600);

  delay(5000);  // oddly, this delay seems necessary in order
                // for the menu to print in setup()

  printMenu();

}  // END SETUP *****************************************************


void printMenu(void) 
{
  Serial.println(F(""));
  Serial.println(F("---------------------- MENU ----------------------"));
  Serial.println(F("Basics"));
  Serial.println(F("m - Print Menu"));
  Serial.println(F("A - Turn ON SIMCOM"));
  Serial.println(F("Z - Turn OFF SIMCOM"));
  Serial.println(F("i - Initialize SIMCOM chip"));
  Serial.println(F("k - Check for OK from SIMCOM chip"));
  Serial.println(F("r - Check reg status of SIMCOM chip"));
  Serial.println(F("c - Check network provider to SIMCOM chip"));
  Serial.println(F("b - Check battery voltage to SIMCOM chip"));
  Serial.println(F("s - Send an AT command to SIMCOM chip"));
  Serial.println(F("SMS"));
  Serial.println(F("t - Send text message"));
  Serial.println(F("GPS"));
  Serial.println(F("G - Turn GPS ON"));
  Serial.println(F("g - Turn GPS OFF"));
  Serial.println(F("l - Get location"));
  Serial.println(F("? - Send location in text message"));
  Serial.println(F("---------------------------------------------------"));
  Serial.println(F(""));
}


// START LOOP **************************************************

void loop() 
{
  if ( Serial.available() )       // check if command typed in serial monitor
  {     
    command = Serial.read();      // read just first char of input
    Serial.println();
    Serial.print(command);        // print char to monitor
    Serial.print(" - ");
    while (Serial.available())    // dump any remaining chars 
    {  
      Serial.read();
    }
  }


  switch (command)
  {
    case 'm':
      Serial.println("Print Menu");
      printMenu();
      break;
    case 'A':
      Serial.println("Turn ON SIMCOM");
      turn_SIMCOM_ON();
      break;
    case 'Z':
      Serial.println("Turn OFF SIMCOM");
      turn_SIMCOM_OFF();
      break;
    case 'i':
      Serial.println("Initialize SIMCOM");
      initialize_SIMCOM();
      break;
    case 'k':
      Serial.println("Check for OK from SIMCOM chip");
      Serial1.println("AT");
      break;
    case 'r':
      Serial.println("Check reg status of SIMCOM chip");
      Serial1.println("AT+CREG?");
      break;
    case 'c':
      Serial.println("Check network provider to SIMCOM chip");
      Serial1.println("AT+COPS?");
      break;
    case 'b':
      Serial1.println("AT+CBC");
      break;
    case 's':
      Serial.println(F("Enter an AT command:"));
      while (Serial.available() == 0) {  }        // Wait for user input
      AT_String = Serial.readStringUntil('\n');   // Read up to new line char
      delay (100);
      Serial1.println(AT_String);
      break;
    case 't':
      Serial.println(F("Enter message:"));
      while (Serial.available() == 0) {  }        // Wait for user input
      SMS_String = Serial.readStringUntil('\n');  // Read up to new line char
      Serial.println(SMS_String);
      send_SMS(SMS_String);
      break;
    case 'G':
      Serial.println(F("Turn GPS ON"));
      Serial1.println("AT+CGNSPWR=1");
      break;
    case 'l':
      get_GPS_LatLong();
      break;
    case '?':
      get_GPS_LatLong();
      send_SMS(LatLongURL);
      break;
    case 'X':
      // do nothing
      break;
  } // end switch

  command = 'X';  // set case to 'X', will do nothing


  if ( Serial1.available() > 0 )
  {
    SIMCOM_String = Serial1.readStringUntil('\n');
    Serial.println(SIMCOM_String);

    if ( SIMCOM_String.startsWith("?") )
    {
      Serial.println("Received request to send location");
      command = '?';
    }
  }

  blink_LED();

} 

// END LOOP **************************************************


void turn_SIMCOM_ON(void)               // sequence to turn on SIMCOM
{
  Serial.println(F("Turn On SIMCOM"));
  digitalWrite(LED_BUILTIN, HIGH);      // turn on LED
  digitalWrite(SIMCOM_KEY, LOW);        // KEY to ground
  delay(2000);                          // 2 seconds
  digitalWrite(SIMCOM_KEY, HIGH);       // KEY to high
  digitalWrite(LED_BUILTIN, LOW);       // turn off LED
}

void turn_SIMCOM_OFF(void)              // sequence to turn off SIMCOM
{
  Serial.println(F("Turn Off SIMCOM"));
  digitalWrite(LED_BUILTIN, HIGH);      // turn on LED
  digitalWrite(SIMCOM_KEY, LOW);        // KEY to ground
  delay(2000);                          // 2 seconds
  digitalWrite(SIMCOM_KEY, HIGH);       // KEY to high
  digitalWrite(LED_BUILTIN, LOW);       // turn off LED
}

void initialize_SIMCOM(void)            // initialize SIMCOM chip
{            
  Serial.println(F("Turn on echo"));
  Serial1.println("ATE1");
  Serial.println(Serial1.readString());
  Serial.println(F("Put SMS in text mode"));
  Serial1.println("AT+CMGF=1");
  Serial.println(Serial1.readString());
  Serial.println(F("Route SMS message to terminal"));
  Serial1.println("AT+CNMI=2,2,0,0,0");
  Serial.println(Serial1.readString());
}

void send_SMS(String SMS_String) 
{                                  
  boolean ok_result = false;
  Serial.println(F("Send SMS"));
  Serial1.print("AT+CMGS=\"" + Cell_Num + "\"");
  Serial1.write(0x0D);  // '\r'
  Serial1.flush();
  Serial1.setTimeout(1000);  //1s

  if (true == Serial1.find(StringGreaterThanSign))
  {
    Serial1.print(SMS_String);  // text above link
    Serial1.write(0x1A);        // send ctrl-q
    Serial1.flush();            // empty buffer
    Serial1.setTimeout(10000);  // wait 10 seconds for text to send and receive OK from SIMCOM
    while (1)
    {
      ok_result = Serial1.find(StringOK);
      if (true == ok_result)
      {
        Serial.println(F("Sending SMS end"));
        break;
      }
    }
  }
}


void get_GPS_LatLong(void) 
{
  Serial.println(F("Get GPS coordinates"));
  Serial1.println("AT+CGNSINF");
  Serial1.flush();
  delay(1000);

  // below looks odd, but avoids compile "warnings" if you use ': ', ie, too many characters
  String GPS_parse = Serial1.readStringUntil(':'); // tosses initial characters
  GPS_parse = Serial1.readStringUntil(' '); // toss space before field for GPS ON?

  // GPS ON?
  GPS_ON = Serial1.readStringUntil(',');
  Serial.print("GPS ON? : ");
  Serial.println(GPS_ON);

  // GPS Fix?
  GPS_Fix = Serial1.readStringUntil(',');
  Serial.print("GPS Fix?: ");
  Serial.println(GPS_Fix);

  // GPS Time
  GPS_Time = Serial1.readStringUntil(',');
  Serial.print("GPS Time: ");
  Serial.println(GPS_Time);

  // GPS Latitude
  GPS_Lat = Serial1.readStringUntil(',');
  Serial.print("GPS Lat : ");
  Serial.println(GPS_Lat);

  // GPS Longitue
  GPS_Long = Serial1.readStringUntil(',');
  Serial.print("GPS Long: ");
  Serial.println(GPS_Long);

  // GPS Altitude
  GPS_Alt = Serial1.readStringUntil(',');
  Serial.print("GPS Alt : ");
  Serial.println(GPS_Alt);

  GPS_parse = Serial1.readStringUntil('\n');  // toss remaining characters

  // format URL for location
  // google maps URL format example
  // http://maps.google.com/?q=38.889484,-77.035278
  LatLongURL = "http://maps.google.com/?q=" + GPS_Lat + "," + GPS_Long;
  Serial.println(LatLongURL);
}


void blink_LED(void) 
{
  currentMillis = millis();

  if (ledState == LOW) 
  {
    if (currentMillis - previousMillis >= interval_OFF) 
    {
      previousMillis = currentMillis;
      ledState = HIGH;
    }
  } else {
    if (currentMillis - previousMillis >= interval_ON) 
    {
      previousMillis = currentMillis;
      ledState = LOW;
    }
  }
  digitalWrite(LED_BUILTIN, ledState);
}
