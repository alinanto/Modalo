/* Written by ALIN ANTO

POWER GRID CORPORATION OF INDA LIMITED

License:Restricted

No part/piece may be reused without explicit permission of POWERGRID */

// pending work
/*
1) start stop logging time from config file
2) JSON files for other device makes
3) WINAPI main for hiding console window
4) preprocessor directives for supporting verbose mode
5) Add functionality for polling multiple devices

/*  Completed Work
1) Fixed bug in file interval - Active thorughout the valid minute
2) Add plant code and asset ID into config file
3) change time stamp to UTC with Z terminator
4) Add plant code and asset ID into log file name
5) log file path change to dynamically read from config file
6) Cleaned and optimised token validation function
*/
// windows runtime libraries
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <stdio.h>

// headers to my dll libraris
#include "modbus.h"
#include "modaloParse.h"
#include "modaloError.h"
// #include "cJSON.h"

// program static configuration
#define CONFIGFILE "../config/modalo.conf"
#define MODBUS_MAX_RETRY 5
#define MODBUS_RETRY_INTERVAL 1000 // in milliseconds (ms)
#define MODALO_SLEEP 900 // sleep time of loop in ms(exactly 1s might overshoot and skip a second)
#define MODALO_SLEEP_MORE 100 // sleep time in case 1s did not elapse (in ms)

// function definitions
modbus_t* initialiseModbus(CONFIG * config);
int readReg(modbus_t* ctx, REG* reg);
int writeLogFile(char* logFileName,char* logFilePath,MAP map,struct tm *UTCTimeS);
uint16_t reverseBits(uint16_t num);

//main function
int main(int argc, char *argv[])
{
  // variables related to time
  time_t seconds=0;      // for seconds holding in windows time API
	struct tm *localTimeS, *UTCTimeS; // for time holding structure in windows time API

  // variables related to logfile
  char logFileName [FILEPATHSIZE] = "modalo_PPPP_AAAAAAAA_YYYY_MM_DD_HH_MM_SS.csv";
  FILE *logFileHandle = NULL;  // Handle for holding log file

  // variables related to configuration and mapping table and modbus
  CONFIG config; // for storing configuration VALUES
  modbus_t *ctx=NULL; // modbus context pointer
  char mapFileName [FILEPATHSIZE] = "../config/SOLIS.json";
  MAP map; // mapfile for reg mappings
  int regIndex=0; // index for register

  printf("Parsing configuration File: %s\n",CONFIGFILE);
  if(!parseModaloConfigFile(&config,CONFIGFILE)) // parse failed
  {
    modaloPrintLastError();
    printf("Continuing with default configuration.\n");
  }
  printModaloConfig(config);

  sprintf(mapFileName,"../config/%s.json",config.device[0].make);
  printf("Parsing json file for reg map: %s\n",mapFileName);
  map = parseModaloJSONFile(mapFileName,config.device[0].model);
  if(map.reg == NULL){
    modaloPrintLastError();
    return -1;
  }
  printModaloMap(map);

  printf("\nInitialsing modbus connection.\n");
  ctx=initialiseModbus(&config);
	if(ctx==NULL) //modbus failed to initialise
  {
    modaloPrintLastError();
    return -1;
  }

  //modbus initialised successfully
  seconds=time(NULL); // update time
  localTimeS = localtime(&seconds); // update time structure with local time
  UTCTimeS = gmtime(&seconds); //update time structure with UTC Time

  sprintf(logFileName,"modalo_%d_%s_%04d_%02d_%02d_%02d_%02d_%02d.csv",
    config.device[0].plantCode, // plant code
    config.device[0].assetID,   // asset ID
    UTCTimeS->tm_year+1900, // The number of years since 1900
    UTCTimeS->tm_mon+1,     // month, range 0 to 11
    UTCTimeS->tm_mday,      // day of the month, range 1 to 31
    UTCTimeS->tm_hour,      // hours, range 0 to 23
    UTCTimeS->tm_min,       // minutes, range 0 to 59
    UTCTimeS->tm_sec);      //update the ISO 8601 time strings

  printf("New File: %s%s\n",config.logFilePath,logFileName);
  seconds = 0; // reset seconds to enter loop without delay

  do{
    while(time(NULL)==seconds)  // if time did not increment
      Sleep(MODALO_SLEEP_MORE);      // wait a bit extra until next second

    // if seconds incremented (code below acts on 1 sec timer roughly - never double counts or under counts)
    seconds=time(NULL); // update time
    localTimeS = localtime(&seconds); // update time structure with local time
    UTCTimeS = gmtime(&seconds); // update time structure with UTC time

    if(localTimeS->tm_sec == 0) { // 1 minute timer
      if((localTimeS->tm_min+(localTimeS->tm_hour*60)-config.startLog)%config.fileInterval == 0) { //file interval starts
        sprintf(logFileName,"modalo_%d_%s_%04d_%02d_%02d_%02d_%02d_%02d.csv",
          config.device[0].plantCode, // plant code
          config.device[0].assetID,   // asset ID
          UTCTimeS->tm_year+1900, // The number of years since 1900
          UTCTimeS->tm_mon+1,     // month, range 0 to 11
          UTCTimeS->tm_mday,      // day of the month, range 1 to 31
          UTCTimeS->tm_hour,      // hours, range 0 to 23
          UTCTimeS->tm_min,       // minutes, range 0 to 59
          UTCTimeS->tm_sec);      //update the ISO 8601 time strings

        printf("New File: %s\n",logFileName);
      } // file interval ends.
    } // 1 minute timer ends

    if(seconds%config.pollInterval == 0) { //poll interval starts
      if(!readReg(ctx,&map.reg[regIndex])) modaloPrintLastError();
      if(++regIndex==map.mapSize) regIndex=0; // reset index
    } // poll interval ends.

    if(seconds%config.sampleInterval == 0) { //sample interval starts
      if(!writeLogFile(logFileName,config.logFilePath,map,UTCTimeS)) {
        modaloPrintLastError();
      }
    } // sample interval ends.

    Sleep(MODALO_SLEEP); // sleep for defined time
	}while(1); // 1 sec loop ends

  // lose the connection and free MAP
  modbus_close(ctx);
  modbus_free(ctx);
  freeMAP(map);
  return 0;

}

modbus_t* initialiseModbus(CONFIG * config)
{
  modbus_t* ctx = NULL; // modbus context pointer
  int count=0;
  char winComPort[16] = "\\\\.\\"; //buffer to hold comport in windows format
  char error[MODALO_ERROR_MAXLENGTH]=""; //buffer to hold error

  strcat(winComPort,config->port); // converting com port to windows format
	do{
    memset(error,'\0',sizeof(char)*MODALO_ERROR_MAXLENGTH); //clear error for next try
		ctx = modbus_new_rtu(winComPort,
      config->baud,
      config->parity,
      config->dataBits,
      config->stopBits);
		if (ctx == NULL)
		{
	    modaloSetLastError(EMODBUS_INIT,"Unable to create the libmodbus context!");
		}
    else if(modbus_connect(ctx) == -1 )
		{
	    sprintf(error,"Unable to connect to %s, ERROR CODE:%s",config->port,modbus_strerror(errno));
      modaloSetLastError(EMODBUS_INIT,error);
	    modbus_free(ctx);
      ctx=NULL;
		}
    else if(modbus_set_slave(ctx, config->device[0].slaveID) == -1)
    {
      sprintf(error,"Unable to set slave, ERROR CODE:%s",modbus_strerror(errno));
      modaloSetLastError(EMODBUS_INIT,error);
	    modbus_free(ctx);
      ctx=NULL;
    }
    count++;
		Sleep(MODBUS_RETRY_INTERVAL);
	}while(ctx == NULL && count<MODBUS_MAX_RETRY); //check for failure and max retries
  return ctx;
}

int readReg(modbus_t* ctx, REG* reg)
{
  char error[MODALO_ERROR_MAXLENGTH] = "";
  int mResult = 0;

  // function code 3: Read Holding Register
  if(reg->functionCode==3) mResult = modbus_read_registers(ctx,reg->regAddress,reg->regSizeU16,reg->valueU16);

  // function code 4: Read input register
  if(reg->functionCode==4) mResult = modbus_read_input_registers(ctx,reg->regAddress,reg->regSizeU16,reg->valueU16);

  if(mResult != reg->regSizeU16)	{ // error reading register
  	sprintf(error, "Reading registers failed with CODE: %s",modbus_strerror(errno));
  	modbus_flush(ctx);
    modaloSetLastError(EMODBUS_READ,error);
  	return 0;
  }

  if(reg->byteReversed) { // reverse the bytes
    uint16_t temp;
    temp = reg->valueU16[0];
    reg->valueU16[0] = reg->valueU16[1];
    reg->valueU16[1] = temp;
  }
  if(reg->bitReversed) { // reverse the Bits
    reg->valueU16[0] = reverseBits(reg->valueU16[0]);
    reg->valueU16[1] = reverseBits(reg->valueU16[1]);
  }

  if(reg->regSizeU16 == 2) { // U32
    reg->value = (float)((reg->valueU16[0]*65536 + reg->valueU16[1]) * reg->multiplier)/ reg->divisor;
    printf("REGNAME:%s, RAWVALUE:0x%04X%04X, VALUE:%0.2f\n",
      reg->regName,
      reg->valueU16[0],
      reg->valueU16[1],
      reg->value);
    fflush(stdout);
    return 1;
  }
  // U16
  reg->value = (float)(reg->valueU16[0] * reg->multiplier)/ reg->divisor;
  printf("REGNAME:%s, RAWVALUE:0x%04X, VALUE:%0.2f\n",
    reg->regName,
    reg->valueU16[0],
    reg->value);
	fflush(stdout);
  return 1;
}

int writeLogFile(char* logFileName,char* logFilePath,MAP map,struct tm *UTCTimeS)
{
  FILE* logFileHandle=NULL;  // Handle for holding log file
  int count=0; // for counting the no. of registers written
  int fResult = 0; // for error checking fprintf
  char fullFileString [2*FILEPATHSIZE] = "";

  sprintf(fullFileString,"%s%s",logFilePath,logFileName);
  logFileHandle = fopen(fullFileString,"a"); // open log file in append mode
  if (logFileHandle == NULL) {
    modaloSetLastError(ELOG_FILE,"Unable to open log file!");
    return 0; //error opening files
  }
  // file opened successfully
  fseek(logFileHandle,0,SEEK_END); // goto end of file
  if(ftell(logFileHandle)==0) { // size of file is 0, now we can write file header
    fResult=fprintf(logFileHandle,"Time_Stamp_(ISO8601)"); // timestamp is always included
    if(fResult<1) { // error writing to file
      modaloSetLastError(ELOG_FILE,"Unable to write to log file!");
      fclose(logFileHandle);
      return 0;
    }
    while(count<map.mapSize) { // loop over each register
      fResult=fprintf(logFileHandle,", %s",map.reg[count].regName); // print register Name
      if(fResult<1) { // error writing to file
        modaloSetLastError(ELOG_FILE,"Unable to write to log file!");
        fclose(logFileHandle);
        return 0;
      }
      count++; // increment to next register
    }
    fResult=fprintf(logFileHandle,"\n"); // new line after headers
    if(fResult<1) { // error writing to file
      modaloSetLastError(ELOG_FILE,"Unable to write to log file!");
      fclose(logFileHandle);
      return 0;
    }
  } // header completed

  //timestamp is column 1: always included ISO 8601 string
  fResult=fprintf(logFileHandle,"%04d-%02d-%02dT%02d:%02d:%02dz",
    UTCTimeS->tm_year+1900, // The number of years since 1900
    UTCTimeS->tm_mon+1,     // month, range 0 to 11
    UTCTimeS->tm_mday,      // day of the month, range 1 to 31
    UTCTimeS->tm_hour,      // hours, range 0 to 23
    UTCTimeS->tm_min,       // minutes, range 0 to 59
    UTCTimeS->tm_sec);      // seconds, range 0 to 59
  if(fResult<1) { // error writing to file
    modaloSetLastError(ELOG_FILE,"Unable to write to log file!");
    fclose(logFileHandle);
    return 0;
  }
  count=0; // reset count before loop
  while(count<map.mapSize) {  // loop over each register
    fResult=fprintf(logFileHandle,", %0.2f",map.reg[count].value); // print register value
    if(fResult<1) { // error writing to file
      modaloSetLastError(ELOG_FILE,"Unable to write to log file!");
      fclose(logFileHandle);
      return 0;
    }
    count++; // increment to next register
  }
  fResult=fprintf(logFileHandle,"\n"); // new line after headers
  if(fResult<1) { // error writing to file
    modaloSetLastError(ELOG_FILE,"Unable to write to log file!");
    fclose(logFileHandle);
    return 0;
  }
  fclose(logFileHandle);
  return 1; // write successfull
}

// Function to reverse bits of num
uint16_t reverseBits(uint16_t num)
{
    int count = 15;
    uint16_t reverse_num = num;

    num >>= 1;
    while (num) {
        reverse_num <<= 1;
        reverse_num |= num & 1;
        num >>= 1;
        count--;
    }
    reverse_num <<= count;
    return reverse_num;
}