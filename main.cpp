/* Logan Scholz DEC-24-2021

***********************************************
*** MANUAL URI NDEF NTAG213 WRITING UTILITY ***
***********************************************

** These comments assume a familiarity with the 
** NFC Data Exchange Format (NDEF)
** https://learn.adafruit.com/adafruit-pn532-rfid-nfc/ndef
** thank you adafruit

*********Summary of utility: 
 - Write a bit.ly (or other reasonably small
   URL) to MIFARE Ultralight NTAG213 RFID sticker
   using an RFID-RC522 14443A reader/writer hooked
   up to a Teensy 4.1 (or some uC in the Arduino
   ecosystem.. mega, uno, esp32..)
 - URL will be auto-opened on an Android phone
   with NFC turned on (probably doesn't work
   with iPhone)

*********Necessary Libraries:
 - SPI.h     :: included w/ platformio
 - MFRC522.h :: https://github.com/miguelbalboa/rfid

*********MFRC522 <-> Teensy 4.1 hookup:
 - SS    = pin 10
 - RESET = pin  9
note: this code should work for any arduino-family board,
      just correctly hook up the SPI.

*********General Notes on MIFARE Ultralight:
 1. Ultralight seems to have 11 writeable pages. 4-15 (inclusive)
    - 44 bytes - 7 bytes for NDEF header = 37 bytes = 37 max character length URL
    - bit.ly helps to get around this limitation

*********General Notes on NDEF Format:

Example NDEF 7 Byte Header for bit.ly/3mzlLb1
0x03, 0x13, 0xD1, 0x01,  || Byte 0,1,2,3
0x0F, 0x55, 0x02, 0xXX   ||      4,5,6,..

Byte 0: TNF. 0x03 means this NDEF message is a URI
Byte 1: Seems to be URL length (in bytes) of the entire URL minus the
        "https://" part + 1 (??? could be coincidence but this
        works consistently so far)  
Byte 2: Keep this 0xD1
Byte 3: Keep this 0x01
Byte 4: Seems to be the URL length (in bytes) of the entire URL minus
        the "https://www." part + 1
Byte 5: Keep this 0x55 (ASCII: "U")
Byte 6: 0x02 corresponds to the link having "https://www." at the start
        (this way we don't have to waste the space putting it in 
        ourselves)

Here is a working example URI NDEF Record for MIFARE Ultralight
that goes to https://www.hackaday.com on an Android phone
03 11 D1 01  _ _ _ _  ||  Underscores are the 7 header bytes, see above
0D 55 02 68  _ _ _ h  || 
61 63 6B 61  a c k a  ||  hackaday.com (12 chars so --> byte 4 length = 13 (0x0D)) // www.hackaday.com (16 chars so --> byte 1 length = 17 (0x11) 
64 61 79 2E  d a y .  ||
63 6F 6D FF  c o m    ||  note: all 0xFF are garbage and unrelated
FF FF FF FF           ||
FF FF FF FF           ||

***********************************************
*/

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  10
#define RST_PIN  9

MFRC522 RFID(SS_PIN, RST_PIN);  // Create MFRC522 instance

MFRC522::StatusCode status;     // variable to get card status

boolean write_record = true;   // set this false to just read and print the internal card contents

byte uri_record_addr = 0x04;    // Write the record starting at Page 4. 
                                // (Pages 0-3 are for internal tag use)



/*
*******************************************************************
***************** URI RECORD CONSTRUCTION SECTION *****************
*******************************************************************
Link contained in NDEF: https://www.bit.ly/3mzlLb1
(redirects to https://www.github.com/hexadecimator)
*/
byte uri_record[25] = 
{
  0x03, 0x13, 0xD1, 0x01,         
  0x0F, 0x55, 0x02, 0x62, 
  0x69, 0x74, 0x2E, 0x6C,
  0x79, 0x2F, 0x33, 0x6D,
  0x7A, 0x6C, 0x4C, 0x62,
  0x31, 0xFF, 0xFF, 0xFF
};
int uri_record_size = sizeof(uri_record);
// *******************************************************************
// *******************************************************************



void setup()
{
    Serial.begin(115200);
    SPI.begin();
    RFID.PCD_Init();
    delay(100);
    Serial.println(F("*** [INFO] ENTERING MAIN LOOP ***"));
    Serial.println();
}

void loop()
{
    if ( ! RFID.PICC_IsNewCardPresent()) return; // Look for new cards
    if ( ! RFID.PICC_ReadCardSerial()) return; // Select one of the cards

    if(write_record)
    {
        for (int i = 0; i < uri_record_size; i++) 
        {
            //data is writen in blocks of 4 bytes (4 bytes per page)
            status = (MFRC522::StatusCode) RFID.MIFARE_Ultralight_Write(uri_record_addr+i, &uri_record[i*4], 4);
            if (status != MFRC522::STATUS_OK) 
            {
                Serial.print(F("MIFARE_Read() failed: "));
                Serial.println(RFID.GetStatusCodeName(status));
                return;
            }
        }

        Serial.println();
        Serial.println(F("MIFARE_Ultralight_Write() OK "));
        Serial.println(); Serial.println();
        
    }

    Serial.println(F("***********************************"));
    Serial.println(F("***** Total Internal Contents *****"));
    RFID.PICC_DumpToSerial(&RFID.uid);
    Serial.println();

    RFID.PICC_HaltA();
}