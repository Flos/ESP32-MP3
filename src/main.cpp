#include <Arduino.h>
#include <Wifi.h>
#include <SPIFFS.h>

// SD Card
#include <SPI.h>
#include <SD.h>
#include <FS.h>

// Music
#include "AudioGeneratorMP3.h"
#include "AudioFileSourceSD.h"
#include "AudioOutputI2S.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorWAV.h"

AudioGeneratorMP3 *mp3;
AudioFileSourceSD *file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  (void)cbData;
  Serial.printf("ID3 callback for: %s = '", type);

  if (isUnicode) {
    string += 2;
  }
  
  while (*string) {
    char a = *(string++);
    if (isUnicode) {
      string++;
    }
    Serial.printf("%c", a);
  }
  Serial.printf("'\n");
  Serial.flush();
}

// Called when there's a warning or error (like a buffer underflow or decode hiccup)
void StatusCallback(void *cbData, int code, const char *string)
{
    const char *ptr = reinterpret_cast<const char *>(cbData);
    // Note that the string may be in PROGMEM, so copy it to RAM for printf
    char s1[64];
    strncpy_P(s1, string, sizeof(s1));
    s1[sizeof(s1) - 1] = 0;
    Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
    Serial.flush();
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void setup()
{
    Serial.begin(9600);

    Serial.print("\nInitializing SD card...");
    if(!SD.begin()){
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);

    listDir(SD, "/", 2);
    Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));

    out = new AudioOutputI2S();
    file = new AudioFileSourceSD("/2.MP3");

    Serial.println("ID3 tag");
    id3 = new AudioFileSourceID3(file);
    id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");

    mp3 = new AudioGeneratorMP3();
    mp3->RegisterStatusCB(StatusCallback, (void *)"mp3");
    
    Serial.println("ID3 beginn");
    mp3->begin(id3, out);
};

void loop()
{
    Serial.printf("loop start\n");
    if (mp3->isRunning()) {
        Serial.println( "mp3 is running");
        if (!mp3->loop()) {
            mp3->stop();
            delete mp3;
            id3->close();
            delete id3;
            file->close();
            delete file;
            out->stop();
            delete out;
            Serial.print( "mp3 is stoped");
        } 
    } else {
        Serial.printf("MP3 done\n");
        
        
        delay(200);
        file = new AudioFileSourceSD("/1.MP3");
        id3 = new AudioFileSourceID3(file);
        id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
        mp3 = new AudioGeneratorMP3();
        mp3->RegisterStatusCB(StatusCallback, NULL);
        Serial.println("ID3 beginn");
        out = new AudioOutputI2S();
        mp3->begin(id3, out);
    }
    delay(200);
    Serial.printf("loop end\n");
};
