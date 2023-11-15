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
6) Implement log file name as dynamically allocated and only pointer part of device config

/*  Completed Work
1) Fixed bug in file interval - Active thorughout the valid minute
2) Add plant code and asset ID into config file
3) change time stamp to UTC with Z terminator
4) Add plant code and asset ID into log file name
5) log file path change to dynamically read from config file
6) Cleaned and optimised token validation function
7) Fixed Bug while printing parity
*/
// windows runtime libraries
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

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
  int startLoggingFLag=0; // Flag for starting logging

  // variables related to configuration and mapping table and modbus
  CONFIG config; // for storing modbus configuration, logging configuration and device configuration 
  modbus_t *ctx=NULL; // modbus context pointer
  char mapFileName [MAXNAMESIZE+20] = "../config/SOLIS.json";
  
  // setbuf(stdout,NULL); // set std out to unbuffered

  // parse the configuration file
  printf("Parsing configuration File: %s\n",CONFIGFILE);
  if(!parseModaloConfigFile(&config,CONFIGFILE)) // parse failed
  {
    modaloPrintLastError();
    printf("Continuing with default configuration.\n");
  }
  printModaloConfig(config);

  seconds=time(NULL); // update time
  localTimeS = localtime(&seconds); // update time structure with local time
  UTCTimeS = gmtime(&seconds); //update time structure with UTC Time

  _MODALO_forEachDevice(device,config) { // MACRO to loop over each device in config structure
    if(device->slaveID == 0) continue;   // skip over empty device declarations

    // starting to read map files and save to device configuration structure
    sprintf(mapFileName,"../config/%s.json",device->make);
    printf("Parsing json file for reg map: %s\n",mapFileName);
    device->map = parseModaloJSONFile(mapFileName,device->model); // get map from JSON file
    if(device->map.reg == NULL){ // handle error
      modaloPrintLastError();
      freeMAP(config);
      system("pause");
      return -1;
    }

    // update log file name for each device first use
    sprintf(device->logFileName,"modalo_%d_%s_%04d%02d%02dT%02d%02d%02dz.csv",
      device->plantCode,      // plant code
      device->assetID,        // asset ID
      UTCTimeS->tm_year+1900, // The number of years since 1900
      UTCTimeS->tm_mon+1,     // month, range 0 to 11
      UTCTimeS->tm_mday,      // day of the month, range 1 to 31
      UTCTimeS->tm_hour,      // hours, range 0 to 23
      UTCTimeS->tm_min,       // minutes, range 0 to 59
      UTCTimeS->tm_sec);      //update the ISO 8601 time strings
  }

  // initialise modbus  
  printf("\nInitialsing modbus connection.\n");
  ctx=initialiseModbus(&config);
	if(ctx==NULL) //modbus failed to initialise
  {
    modaloPrintLastError();
    freeMAP(config);
    system("pause");
    return -1;
  }

  MAP map;
  map = config.device[0].map; //temporary fix, should be removed after multiple read success

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
        _MODALO_forEachDevice(device,config) { // MACRO to loop over each device in config structure
          if(device->slaveID == 0) continue;   // skip over empty device declarations

          // update log file name for each device
          sprintf(device->logFileName,"modalo_%d_%s_%04d%02d%02dT%02d%02d%02dz.csv",
            device->plantCode,      // plant code
            device->assetID,        // asset ID
            UTCTimeS->tm_year+1900, // The number of years since 1900
            UTCTimeS->tm_mon+1,     // month, range 0 to 11
            UTCTimeS->tm_mday,      // day of the month, range 1 to 31
            UTCTimeS->tm_hour,      // hours, range 0 to 23
            UTCTimeS->tm_min,       // minutes, range 0 to 59
            UTCTimeS->tm_sec);      //update the ISO 8601 time strings
          printf("\n\nNew Log File: %s%s\n",config.logFilePath,device->logFileName);
        }
      } // file interval ends.
    } // 1 minute timer ends

    if(seconds%config.pollInterval == 0) { //poll interval starts
      if(map.regIndex>=map.mapSize) map.regIndex=0; // index overflow => reset index
      if(!readReg(ctx,&map.reg[map.regIndex++])) modaloPrintLastError(); // read reg error
      else { // read success
        system("cls");  // clear screen
        printModaloMap(config); //print map
      }
    } // poll interval ends.

    if(seconds%config.sampleInterval == 0) { //sample interval starts
      if(!writeLogFile(config.device[0].logFileName,config.logFilePath,map,UTCTimeS)) {
        modaloPrintLastError();
      }
    } // sample interval ends.

    Sleep(MODALO_SLEEP); // sleep for defined time
	}while(1); // 1 sec loop ends

  // lose the connection and free MAP
  modbus_close(ctx);
  modbus_free(ctx);
  freeMAP(config);
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
  	sprintf(error, "Reading register %s failed with CODE: %s",reg->regName,modbus_strerror(errno));
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

  if(reg->regSizeU16 == 2) // U32
    reg->value = (((double)reg->valueU16[0]*65536 + (double)reg->valueU16[1]) * (double)reg->multiplier) / (double)reg->divisor;
  else // U16
    reg->value = ((double)reg->valueU16[0] * (double)reg->multiplier)/ (double)reg->divisor;

  return 1; // read success
}

int writeLogFile(char* logFileName,char* logFilePath,MAP map,struct tm *UTCTimeS)
{
  FILE* logFileHandle=NULL;  // Handle for holding log file
  int count=0; // for counting the no. of registers written
  int fResult = 0; // for error checking fprintf
  char fullFileString [FILENAMESIZE+FILEPATHSIZE] = "";

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