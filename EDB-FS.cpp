/*
  EDB-FS.cpp
  Extended Database Library for Arduino, based on SPIFFS
  http://www.arduino.cc/playground/Code/ExtendedDatabaseLibrary
  JWhiddon - https://github.com/jwhiddon/EDB
  David Pye - ESP8266 / SPIFFS port https://www.github.com/davidmpye/EDB-FS
*/

#include "Arduino.h"
#include "EDB_FS.h"

/**************************************************/
// private functions

// writes EDB_Header
void EDB_FS::writeHead()
{
  dbFile.seek(0, SeekSet);
  dbFile.write(EDB_REC EDB_head, (unsigned long)sizeof(EDB_Header));
  dbFile.flush();
}

// reads EDB_Header
void EDB_FS::readHead()
{
  dbFile.seek(0, SeekSet);
  dbFile.read(EDB_REC EDB_head, (unsigned long)sizeof(EDB_Header));
}

/**************************************************/
// public functions

EDB_FS::EDB_FS()
{

}

// creates a new table and sets header values
EDB_Status EDB_FS::create(const char *name, unsigned long tablesize, unsigned int recsize)
{
  dbFileName = name;
  SPIFFS.begin();
  dbFile = SPIFFS.open(name, "w+");
  if (!dbFile) return EDB_ERROR;

  EDB_table_ptr = sizeof(EDB_Header);
  EDB_head.flag = EDB_FLAG;
  EDB_head.n_recs = 0;
  EDB_head.rec_size = recsize;
  EDB_head.table_size = tablesize;
  writeHead();
  return EDB_OK;
}

EDB_Status EDB_FS::open(const char *name) {
  dbFileName = name;
  SPIFFS.begin();
  dbFile = SPIFFS.open(dbFileName, "r+");
  if (dbFile) return EDB_OK;
  else return EDB_ERROR;
}

// writes a record to a given recno
EDB_Status EDB_FS::writeRec(unsigned long recno, const EDB_Rec rec)
{
  dbFile.seek(EDB_table_ptr + (recno * EDB_head.rec_size), SeekSet);
  dbFile.write(rec, EDB_head.rec_size);
  dbFile.flush();
  return EDB_OK;
}

// reads a record from a given recno
EDB_Status EDB_FS::readRec(unsigned long recno, EDB_Rec rec)
{
  if (recno < 0 || recno > EDB_head.n_recs - 1) return EDB_OUT_OF_RANGE;
  dbFile.seek(EDB_table_ptr + (recno * EDB_head.rec_size), SeekSet);
  dbFile.read(rec, EDB_head.rec_size);
  return EDB_OK;
}

// Deletes a record at a given recno
EDB_Status EDB_FS::deleteRec(unsigned long recno)
{
  if (recno < 0 || recno > EDB_head.n_recs - 1) return  EDB_OUT_OF_RANGE;
  //Buffer
  unsigned char rec[EDB_head.rec_size];
  
  //Create a temporary file
  File tmpFile = SPIFFS.open("db_tmp_file", "a");
  //Leave room for the header.
  tmpFile.seek(sizeof(EDB_Header), SeekSet);
  //Write all the records except this one into the temporary file.
  for (unsigned long i = 0; i < EDB_head.n_recs; ++i) {
    if (i != recno) {
      readRec(i, rec);
      tmpFile.write(rec, EDB_head.rec_size);
    }
  }
  tmpFile.close();
  dbFile.close();

  SPIFFS.remove(dbFileName); //delete original file.
  SPIFFS.rename("db_tmp_file", dbFileName);

  dbFile = SPIFFS.open(dbFileName, "r+");
  EDB_head.n_recs--;
  writeHead();
  return EDB_OK;
}

// Inserts a record at a given recno, increasing all following records' recno by 1.
// This function becomes increasingly inefficient as it's currently implemented and
// is the slowest way to add a record.
EDB_Status EDB_FS::insertRec(unsigned long recno, EDB_Rec rec)
{
  if (count() == limit()) return EDB_TABLE_FULL;
  if (count() > 0 && (recno < 0 || recno > EDB_head.n_recs - 1)) return EDB_OUT_OF_RANGE;
  if (count() == 0 && recno == 0) return appendRec(rec);

  //Create a temporary file
  File tmpFile = SPIFFS.open("db_tmp_file", "a");
  unsigned char buf[EDB_head.rec_size];
  //Leave room for the header to be written.
  tmpFile.seek((unsigned long)sizeof(EDB_Header), SeekSet);

  for (unsigned long i = 0; i < EDB_head.n_recs; ++i) {
    if (i == recno) {
      //Write this record in to place.
      tmpFile.write(rec, EDB_head.rec_size);
    }
    readRec(i, buf);
    tmpFile.write(buf, EDB_head.rec_size);
  }
  tmpFile.close();
  dbFile.close();

  SPIFFS.remove(dbFileName); //delete original file.
  SPIFFS.rename("db_tmp_file", dbFileName);

  dbFile = SPIFFS.open(dbFileName, "r+");
  EDB_head.n_recs++;
  writeHead();
  return EDB_OK;
}

// Updates a record at a given recno
EDB_Status EDB_FS::updateRec(unsigned long recno, EDB_Rec rec)
{
  if (recno < 0 || recno > EDB_head.n_recs - 1) return EDB_OUT_OF_RANGE;
  writeRec(recno, rec);
  return EDB_OK;
}

// Adds a record to the end of the record set.
// This is the fastest way to add a record.
EDB_Status EDB_FS::appendRec(EDB_Rec rec)
{
  if (EDB_head.n_recs + 1 > limit()) return EDB_TABLE_FULL;
  writeRec(EDB_head.n_recs,rec);
  EDB_head.n_recs++;
  writeHead();
  return EDB_OK;
}

// returns the number of queued items
unsigned long EDB_FS::count()
{
  return EDB_head.n_recs;
}

// returns the maximum number of items that will fit into the queue
unsigned long EDB_FS::limit()
{
   // Thanks to oleh.sok...@gmail.com for the next line
   return (EDB_head.table_size - sizeof(EDB_Header)) / EDB_head.rec_size;
}

// truncates the queue by resetting the internal pointers
void EDB_FS::clear()
{
  readHead();
  create(dbFileName, EDB_head.table_size, EDB_head.rec_size);
}
