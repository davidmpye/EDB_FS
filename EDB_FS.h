/*
  EDB_FS.h
  Extended Database Library for Arduino / SPIFFS
  Modified for efficient use with SPIFFS for ESP8266
  http://www.github.com/davidmpye/EDB

  Non-spiffs ver from http://www.github.com/jwhiddon/EDB
  http://www.arduino.cc/playground/Code/ExtendedDatabaseLibrary
*/

#include <LittleFS.h>

#ifndef EDBFS
#define EDBFS
#define EDB_FLAG B11011011

struct EDB_Header
{
  byte flag;
  unsigned long n_recs;
  unsigned int rec_size;
  unsigned long table_size;
  char version[33]; //32 byte + null terminator
};


typedef enum EDB_Status {
                          EDB_OK,
                          EDB_ERROR,
                          EDB_OUT_OF_RANGE,
                          EDB_TABLE_FULL
};

typedef byte* EDB_Rec;

#define EDB_REC (byte*)(void*)&

class EDB_FS {
  public:
    EDB_FS();
    EDB_Status open(const char *);
    EDB_Status create(const char *, unsigned long, unsigned int);
    EDB_Status close();
    EDB_Status readRec(unsigned long, EDB_Rec);
    EDB_Status deleteRec(unsigned long);
    EDB_Status insertRec(unsigned long, const EDB_Rec);
    EDB_Status updateRec(unsigned long, const EDB_Rec);
    EDB_Status appendRec(EDB_Rec rec);
    unsigned long limit();
    unsigned long count();
    void clear();
	
    EDB_Status setDBVersion(const char *);
    const char *DBVersion();

  private:
    EDB_Header EDB_head;
    void writeHead();
    void readHead();
    EDB_Status writeRec(unsigned long, const EDB_Rec);

    File dbFile;
    const char *dbFileName;
};

#endif
