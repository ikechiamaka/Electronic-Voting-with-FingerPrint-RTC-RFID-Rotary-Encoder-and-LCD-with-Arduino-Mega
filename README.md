# Electronic-Voting-with-FingerPrint-RTC-RFID-Rotary-Encoder-and-LCD-with-Arduino-Mega
This project is an electronic voting system that uses a fingerprint sensor, an RFID reader, and a rotary encoder for navigation. The project also monitors the battery level and displays the battery percentage on an LCD screen.

Here's a summary of the main components and their functions:

Fingerprint sensor (Adafruit_Fingerprint library):
Used for user authentication during the enrollment and voting process.

RFID reader (MFRC522 library): 
Utilized for reading RFID tags. In this project, an RFID tag acts as a master tag to authorize certain actions such as uploading the voting results.

Rotary encoder: 
Provides a user-friendly way to navigate through the menu options displayed on the LCD screen.

LCD screen (LiquidCrystal_I2C library):
Displays the menu options, messages, and battery percentage.

Battery percentage monitoring:
Measures the battery voltage using a voltage divider and maps it to a percentage. The battery percentage is displayed on the LCD screen.

# The project has the following main functionalities:

Enroll: Allows users to enroll their fingerprints. A user is assigned a unique fingerprint ID upon successful enrollment.

Vote: Authenticated users can cast their vote for a candidate. The system ensures that each user can only vote once.

Results: Displays the voting results. The master RFID tag must be present for this option to be available.

Upload: Sends the voting results to a remote server (in this case, ThingSpeak). The master RFID tag must be present for this option to be available.

The code is organized into several functions that handle each part of the process, such as enrolling a fingerprint, casting a vote, checking results, and uploading the results. Additionally, there are functions to manage the menu navigation and battery percentage monitoring.
# Here's the Circuit diagram designed with Fritzing
![image](https://user-images.githubusercontent.com/66757712/233384676-2005be80-e8b9-4e6c-a9fb-eb2b904bf2bc.png)

# Here's the physical setup
![image](https://user-images.githubusercontent.com/66757712/233385389-fa63407a-9f05-4ed6-bc16-b54cdc20a72a.png)

# Here's the Menu Layout
![image](https://user-images.githubusercontent.com/66757712/233385538-65c27861-16eb-4668-a339-d3a50a477126.png)




