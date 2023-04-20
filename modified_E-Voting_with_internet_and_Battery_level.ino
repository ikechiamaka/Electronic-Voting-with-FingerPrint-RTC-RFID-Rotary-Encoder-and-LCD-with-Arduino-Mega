#include <RTClib.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

#include <MFRC522.h>
#define Buzzer ""
#define RedLed ""
#define GreenLed 10
#define RX 8
#define TX 9


constexpr uint8_t RST_PIN = 5;  // Configurable, see typical pin layout
constexpr uint8_t SS_PIN = 53;  // Configurable, see typical pin layout

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
SoftwareSerial esp8266(RX, TX);
#define mySerial Serial1
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

LiquidCrystal_I2C lcd(0x27, 20, 4);
// Initialize the DS3231 RTC module
RTC_DS3231 rtc;



String AP = "Galaxy A1217D2";
String PASS = "1234567890";
String API = "UFXQ8UQ3N4RGEZUW";
String HOST = "api.thingspeak.com";
String PORT = "80";


const int clkPin = 4;
const int dtPin = 6;
const int swPin = 7;

int currentStateCLK;
int previousStateCLK;
int menuOption = 1;
int subMenuOption = 1;
bool inSubMenu = false;
uint8_t id;
bool hasVoted[128] = { false };

int voteCount[] = { 0, 0 };  // Assuming there are two candidates, initialize their vote counts to 0
String Candidate1 = "Muhannad";
String Candidate2 = "Aziz";
const int MAX_VOTERS = 100;      // Define a constant for the maximum number of voters
int votedFingerIDs[MAX_VOTERS];  // Declare the votedFingerIDs array
int numVoters = 0;               // Keep track of the number of voters
unsigned long lastInteraction;
unsigned long idleTimeout = 20000;  // 30 seconds (30,000 milliseconds)
bool displayingDateTime = false;
byte tagID[4];
byte MasterTag[4] = { 0x03, 0x89, 0x9B, 0x92 };  // Master tag

const int batteryPin = A0;
const float R1 = 10000.0;  // Resistance of R1 in Ohms
const float R2 = 10000.0;  // Resistance of R2 in Ohms

const float minBatteryVoltage = 3.0;  // Minimum battery voltage considered 0%
const float maxBatteryVoltage = 4.2;  // Maximum battery voltage considered 100%


void updateMenu();

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  // Initialize the RFID module
  SPI.begin();         // Init SPI bus
  mfrc522.PCD_Init();  // Init MFRC522
  pinMode(Buzzer, OUTPUT);
  pinMode(RedLed, OUTPUT);
  pinMode(GreenLed, OUTPUT);
  digitalWrite(Buzzer, LOW);
  digitalWrite(RedLed, LOW);
  digitalWrite(GreenLed, LOW);
  pinMode(clkPin, INPUT);
  pinMode(dtPin, INPUT);
  pinMode(swPin, INPUT_PULLUP);

  previousStateCLK = digitalRead(clkPin);

  lcd.init();
  lcd.backlight();

  updateMenu();

  finger.begin(57600);
  if (finger.verifyPassword()) {
    lcd.clear();
    lcd.setCursor(0, 1);

    lcd.print("Fingerprint");
    lcd.setCursor(0, 2);
    lcd.print("     ACTIVE! :)");
    delay(1000);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Scroll to Start");
    lcd.setCursor(0, 2);
    lcd.print("--------------->");

  } else {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Fingerprint");
    lcd.setCursor(0, 2);
    lcd.print("   INACTIVE! :(");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Check Connection");
  }
  rtc.begin();
  lastInteraction = millis();
  // Initialize the ESP8266 Wi-Fi module
  esp8266.begin(115200);
  sendCommand("AT", 5, "OK");
  sendCommand("AT+CWMODE=1", 5, "OK");
  sendCommand("AT+CWJAP=\"" + AP + "\",\"" + PASS + "\"", 20, "OK");
}

boolean getID();
void checkResults();


void loop() {
  currentStateCLK = digitalRead(clkPin);
  
  if (currentStateCLK != previousStateCLK || digitalRead(swPin) == LOW) {
    lastInteraction = millis();  // Update the last interaction timestamp
    if (displayingDateTime) {
      displayingDateTime = false;
      updateMenu();  // Refresh menu when returning from displaying date and time
    }
  }else{
      displayingDateTime = true;
  }

  if (currentStateCLK != previousStateCLK  && currentStateCLK == 1) {
    if (digitalRead(dtPin) != currentStateCLK) {
      if (!inSubMenu) {
        menuOption++;
        if (menuOption > 3) menuOption = 1;
      } else {
        subMenuOption++;
        if (subMenuOption > 2) subMenuOption = 1;
      }
    } else {
      if (!inSubMenu) {
        menuOption--;
        if (menuOption < 1) menuOption = 3;
      } else {
        subMenuOption--;
        if (subMenuOption < 1) subMenuOption = 2;
      }
    }
    updateMenu();
  }
  previousStateCLK = currentStateCLK;

  if (digitalRead(swPin) == LOW) {
    if (!inSubMenu) {
      inSubMenu = true;
      subMenuOption = 1;
      updateMenu();
    } else {
      switch (menuOption) {
        case 1:
          if (subMenuOption == 1) {
            enrollFingerprint();
          } else if (subMenuOption == 2) {
            inSubMenu = false;
          }
          break;
        case 2:
          if (subMenuOption == 1) {
            castVote();  //Add this function later when you have the code.
          } else if (subMenuOption == 2) {
            inSubMenu = false;
          }
          break;
        case 3:
          if (subMenuOption == 1) {
            checkResults();
          } else if (subMenuOption == 2) {
            inSubMenu = false;
          }
          break;
          case 4:
          if (subMenuOption == 1) {
            sendVotesToThingSpeak();
          } else if (subMenuOption == 2) {
            inSubMenu = false;
          }
          break;
      }
      updateMenu();
    }
    delay(250);
  }

  // Check for idle time
  if (displayingDateTime && millis() - lastInteraction > idleTimeout) {
    displayDateTime();
 
    displayingDateTime = true;
  }
}

float readBatteryVoltage() {
  float voltage = analogRead(batteryPin) * (5.0 / 1023.0);
  float batteryVoltage = voltage * (R1 + R2) / R2;
  return batteryVoltage;
}

int readBatteryPercentage() {
  float batteryVoltage = readBatteryVoltage();
  float batteryPercentage = (batteryVoltage - minBatteryVoltage) * 100 / (maxBatteryVoltage - minBatteryVoltage);
  return constrain(batteryPercentage, 0, 100);
}




void displayDateTime() {
  DateTime now = rtc.now(); //get the current date and time from RTC
  
  //Display date and time on the LCD screen
  lcd.setCursor(0,1);
  lcd.print("Date: ");
  lcd.print(now.day());
  lcd.print("/");
  lcd.print(now.month());
  lcd.print("/");
  lcd.print(now.year());
  lcd.setCursor(0, 2);
  lcd.print("Time: ");
  if(now.hour() < 10){
    lcd.print("0");
  }
  lcd.print(now.hour());
  lcd.print(":");
  if(now.minute() < 10){
    lcd.print("0");
  }
  lcd.print(now.minute());
  lcd.print(":");
  if(now.second() < 10){
    lcd.print("0");
  }
  lcd.print(now.second());
  //delay(1000); 
}




void updateMenu() {
  if (!inSubMenu) {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("-Enroll");
    lcd.setCursor(10, 1);
    lcd.print("-Vote");
    lcd.setCursor(0, 2);
    lcd.print("-Results");
    lcd.setCursor(10, 2);
    lcd.print("-Upload");

    switch (menuOption) {
      case 1:
        lcd.setCursor(0, 1);
        lcd.print(">Enroll");
        break;
      case 2:
        lcd.setCursor(10, 1);
        lcd.print(">Vote");
        break;
      case 3:
        lcd.setCursor(0, 2);
        lcd.print(">Results");
        break;
      case 4:
        lcd.setCursor(10, 2);
        lcd.print(">Upload");
        break;
    }
  } else {

    switch (menuOption) {
      case 1:
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("-Enroll FP");
        lcd.setCursor(0, 2);
        lcd.print("-Exit");

        switch (subMenuOption) {
          case 1:
            lcd.setCursor(0, 1);
            lcd.print(">Enroll FP");
            break;
          case 2:
            lcd.setCursor(0, 2);
            lcd.print(">Exit");
            break;
        }
        break;
      case 2:
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("-Cast Vote");
        lcd.setCursor(0, 2);
        lcd.print("-Exit");

        switch (subMenuOption) {
          case 1:
            lcd.setCursor(0, 1);
            lcd.print(">Cast Vote");
            break;
          case 2:
            lcd.setCursor(0, 2);
            lcd.print(">Exit");
            break;
        }
        break;
      case 3:
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("-Check Results");
        lcd.setCursor(0, 2);
        lcd.print("-Exit");

        switch (subMenuOption) {
          case 1:
            lcd.setCursor(0, 1);
            lcd.print(">Check Results");
            break;
          case 2:
            lcd.setCursor(0, 2);
            lcd.print(">Exit");
            break;
        }
        break;
    }
  }

 
  // Display the battery percentage at the top right corner
  lcd.setCursor(12, 0);
  lcd.print("Bat:");
  lcd.setCursor(16, 0);
  lcd.print(readBatteryPercentage());
  lcd.print("%");
}


void enrollFingerprint() {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Enroll a fingerprint");
  lcd.setCursor(0, 2);
  lcd.print("Enter ID 1-127");
  delay(3000);

  // Set the initial ID value.
  id = 1;

  // Flag to exit the ID selection loop.
  bool exitIDSelection = false;
  if (!exitIDSelection){
    lcd.clear();
    lcd.setCursor(0, 2);
    lcd.print("Press switch");
  while (!exitIDSelection) {
    currentStateCLK = digitalRead(clkPin);
   lcd.setCursor(0, 1);
   lcd.print("ID: ");
   lcd.print(id);
   lcd.print("   ");    
    if (currentStateCLK != previousStateCLK && currentStateCLK == 1) {
      if (digitalRead(dtPin) != currentStateCLK && currentStateCLK == 1) {
        id++;
        if (id > 127) id = 1;
      } else {
        id++;
        if (id > 127) id = 1;
      }
    }
    previousStateCLK = currentStateCLK;

    if (digitalRead(swPin) == LOW) {
      exitIDSelection = true;
      delay(250);
    }
  }
  }

  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Enrolling ID #");
  lcd.print(id);
  delay(3000);

  getFingerprintEnroll();
}


uint8_t getFingerprintEnroll() {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Place finger on");
  lcd.setCursor(0, 2);
  lcd.print("sensor...");
  delay(2000);

  // Start of the code copied from the original getFingerprintEnroll() function
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #");
  Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");

        break;
      case FINGERPRINT_NOFINGER:
        Serial.println(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        lcd.clear();
        lcd.print("Comm. error");
        delay(2000);
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        lcd.clear();
        lcd.print("Imaging error");
        delay(2000);
        break;
      default:
        Serial.println("Unknown error");
        lcd.clear();
        lcd.print("Unknown error");
        delay(2000);
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("No finger Features");
      lcd.setCursor(0, 2);
      lcd.print("Enroll Again!");
      delay(2000);
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("No finger Features");
      lcd.setCursor(0, 2);
      lcd.print("Enroll Again!");
      delay(2000);
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Remove Finger");

  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID ");
  Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Place same");
  lcd.setCursor(0, 2);
  lcd.print("Finger Again!");

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");

        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("No finger Features");
      lcd.setCursor(0, 2);
      lcd.print("Enroll Again!");
      delay(2000);
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("No finger Features");
      lcd.setCursor(0, 2);
      lcd.print("Enroll Again!");
      delay(2000);
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");
  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID ");
  Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Fingerprint ID");
    lcd.setCursor(0, 2);
    lcd.print(id);
    lcd.print(" enrolled!");
    delay(3000);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}

void castVote() {
  int fingerID;

  // Scan the fingerprint
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Scan your finger");
  delay(2000);

  fingerID = getFingerprintIDez();
  delay(2000);
  if (fingerID >= 0) {
    if (hasAlreadyVoted(fingerID)) {
      digitalWrite(Buzzer, HIGH);
      tone(Buzzer, 1000, 500);
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("Recognised!");
      delay(500);
      lcd.setCursor(0, 2);
      lcd.print("Already voted");
      
      delay(2000);
      digitalWrite(Buzzer, LOW);
    } else {
      digitalWrite(Buzzer, HIGH);
      tone(Buzzer, 1000, 500);
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("Recognised!");
      lcd.setCursor(0, 2);
      lcd.print("scroll to vote->");
      
      delay(1000);
      digitalWrite(Buzzer, LOW);
      // Show the candidates and select one using the encoder
      int selectedCandidate = selectCandidateUsingEncoder();

      // Update the vote count for the selected candidate
      voteCount[selectedCandidate]++;

      // Record the fingerID that has voted
      if (numVoters < MAX_VOTERS) {
        votedFingerIDs[numVoters++] = fingerID;
      }

      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("Vote cast for");
      lcd.setCursor(0, 2);
      lcd.print("Candidate ");
      if (selectedCandidate + 1 == 1) {
        lcd.print(Candidate1);
      } else {
        lcd.print(Candidate2);
      }
      delay(3000);
    }
  } else {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Finger Not Seen!");
    delay(2000);
  }
}



bool hasAlreadyVoted(int fingerID) {
  for (int i = 0; i < numVoters; i++) {
    if (votedFingerIDs[i] == fingerID) {
      return true;
    }
  }
  return false;
}




int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return -1;

  // Found a match!
  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);

  return finger.fingerID;
}


int selectCandidateUsingEncoder() {
  int selectedCandidate = 0;  // Default selection is Candidate A
  bool selectionConfirmed = false;

  while (!selectionConfirmed) {
    currentStateCLK = digitalRead(clkPin);
    if (currentStateCLK != previousStateCLK && currentStateCLK == 1) {
      if (digitalRead(dtPin) != currentStateCLK) {
        selectedCandidate++;
        if (selectedCandidate > 1) selectedCandidate = 0;
      } else {
        selectedCandidate--;
        if (selectedCandidate < 0) selectedCandidate = 1;
      }
      updateCandidateSelection(selectedCandidate);
    }
    previousStateCLK = currentStateCLK;

    if (digitalRead(swPin) == LOW) {
      selectionConfirmed = true;
      delay(250);
    }
  }

  return selectedCandidate;
}

void updateCandidateSelection(int selectedCandidate) {
  lcd.clear();
  if (selectedCandidate == 0) {
    lcd.setCursor(0, 1);
    lcd.print(">1. ");
    lcd.print(Candidate1);
    lcd.setCursor(0, 2);
    lcd.print(" 2. ");
    lcd.print(Candidate2);
  } else {
    lcd.setCursor(0, 1);
    lcd.print(" 1. ");
    lcd.print(Candidate1);
    lcd.setCursor(0, 2);
    lcd.print(">2. ");
    lcd.print(Candidate2);
  }
}




void checkResults() {

  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Scan card");
  delay(10);

  bool cardDetected = false;

  while (!cardDetected) {
    if (getID()) {
      cardDetected = true;
      lcd.clear();
      lcd.setCursor(0, 1);

      if (memcmp(tagID, MasterTag, 4) == 0) {
        digitalWrite(GreenLed, HIGH);
        digitalWrite(Buzzer, HIGH);
        tone(Buzzer, 1000, 500);      
        lcd.print(" Access Granted!");
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print(Candidate1);
        lcd.print(": ");
        lcd.print(voteCount[0]);
        lcd.setCursor(0, 2);
        lcd.print(Candidate2);
        lcd.print(": ");
        lcd.print(voteCount[1]);
        delay(1000);
        digitalWrite(GreenLed, LOW);
      } else {
        lcd.print(" Access Denied!");
        digitalWrite(RedLed, HIGH);
        digitalWrite(Buzzer, HIGH);
        tone(Buzzer, 500, 700);
        delay(1200);
        digitalWrite(Buzzer, HIGH);
        tone(Buzzer, 500, 700);
        delay(1200);
        digitalWrite(Buzzer, HIGH);
        tone(Buzzer, 500, 700);
        delay(700);
        digitalWrite(RedLed, LOW);
        digitalWrite(Buzzer, LOW);
      }
      delay(2000);
    }
  }
}


boolean getID() {
  // Getting ready for Reading PICCs
  if (!mfrc522.PICC_IsNewCardPresent()) {  // If a new PICC placed to RFID reader continue
    return false;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {  // Since a PICC placed get Serial and continue
    return false;
  }

  for (uint8_t i = 0; i < 4; i++) {  // The MIFARE PICCs that we use have 4 byte UID
    tagID[i] = mfrc522.uid.uidByte[i];
  }

  // Print the UID in HEX format
  for (uint8_t i = 0; i < 4; i++) {
    Serial.print(tagID[i], HEX);
    if (i < 3) {
      Serial.print(' ');  // Add a space between bytes
    } else {
      Serial.println();  // Add a newline after the last byte
    }
  }

  mfrc522.PICC_HaltA();  // Stop reading
  return true;
}

void sendVotesToThingSpeak() {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Uploading votes...");

  String getField1 = String(voteCount[0]);
  String getField2 = String(voteCount[1]);

  String getData = "GET /update?api_key=" + API + "&field1=" + getField1 + "&field2=" + getField2;
  sendCommand("AT+CIPMUX=1", 5, "OK");
  sendCommand("AT+CIPSTART=0,\"TCP\",\"" + HOST + "\"," + PORT, 15, "OK");
  sendCommand("AT+CIPSEND=0," + String(getData.length() + 4), 4, ">");
  esp8266.println(getData);
  delay(1500);
  sendCommand("AT+CIPCLOSE=0", 5, "OK");

  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Uploaded Votes");
  lcd.setCursor(0, 2);
  lcd.print("SUCCESSFUL!");
  delay(2000);

  inSubMenu = false;  // Add this line to return to the main menu
}

void sendCommand(String command, int maxTime, char readReplay[]) {
  int countTimeCommand = 0;
  boolean found = false;

  while (countTimeCommand < (maxTime * 1)) {
    esp8266.println(command);
    if (esp8266.find(readReplay)) {
      found = true;
      break;
    }

    countTimeCommand++;
  }

  if (found) {
    Serial.println("Success");
  } else {
    Serial.println("Fail");
  }
}

