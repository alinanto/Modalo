/* Written by ALIN ANTO

POWER GRID CORPORATION OF INDA LIMITED

License:Restricted

No part/piece may be reused without explicit permission of POWERGRID */

// pending work
/*
1) start stop logging time from config file
2) log file path change to dynamically read from config file
3) JSON files for other inverter makes
4) WINAPI main for hiding console window
5) preprocessor directives for supporting verbose mode
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
#define FILEPATHSIZE 64
// function definitions
modbus_t* initialiseModbus(CONFIG * config);
int readReg(modbus_t* ctx, REG* reg);
int writeLogFile(char* logFileName,MAP map,struct tm *timeStruct);
uint16_t reverseBits(uint16_t num);

//main function
int main(int argc, char *argv[])
{
  // variables related to time
  time_t seconds=0;      // for seconds holding in windows time API
	struct tm *timeStruct; // for time holding structure in windows time API

  // variables related to logfile
  char logFileName [FILEPATHSIZE] = "../log/modalo_YYYY_MM_DD_HH_MM_SS.csv";
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

  printf("\nPort: %s\n",config.port);
  printf("Baud: %d\n",config.baud);
  printf("Parity: %d\n",config.parity);
  printf("Stop Bits: %d\n",config.stopBits);
  printf("Inverter Make: %s\n",config.make);
  printf("inverter Model: %s\n",config.model);
  printf("Capacity: %d\n",config.capacity);
  printf("Sample interval: %d\n",config.sampleInterval);
  printf("File interval: %d\n",config.fileInterval);
  printf("Poll interval: %d\n",config.pollInterval);
  printf("Start Log min: %d\n",config.startLog);
  printf("Stop log min: %d\n\n",config.stopLog);

  sprintf(mapFileName,"../config/%s.json",config.make);
  printf("Parsing json file for reg map: %s\n",mapFileName);
  map = parseModaloJSONFile(mapFileName,config.model);
  if(map.reg == NULL){
    modaloPrintLastError();
    return -1;
  }

  int i=0;
  for(i=0;i<map.mapSize;i++) {
    printf("\nReg Name: %s\n",map.reg[i].regName);
    printf("Reg Add: %d\n",map.reg[i].regAddress);
    printf("Reg SizeU16: %d\n",map.reg[i].regSizeU16);
    printf("Byte Reverse: %d\n",map.reg[i].byteReversed);
    printf("Bit Reverse: %d\n",map.reg[i].bitReversed);
    printf("Function Code: %d\n",map.reg[i].functionCode);
    printf("Multiplier:  %d\n",map.reg[i].multiplier);
    printf("Divisor: %d\n",map.reg[i].divisor);
    printf("Moving Avg Filter: %d\n",map.reg[i].movingAvgFilter);
  }

  printf("\nInitialsing modbus connection.\n");
  ctx=initialiseModbus(&config);
	if(ctx==NULL) //modbus failed to initialise
  {
    modaloPrintLastError();
    return -1;
  }


  //modbus initialised successfully
  seconds=time(NULL); // update time
  timeStruct = localtime(&seconds); // update time structure
  sprintf(logFileName,"../log/modalo_%04d_%02d_%02d_%02d_%02d_%02d.csv",
    timeStruct->tm_year+1900, // The number of years since 1900
    timeStruct->tm_mon+1,     // month, range 0 to 11
    timeStruct->tm_mday,      // day of the month, range 1 to 31
    timeStruct->tm_hour,      // hours, range 0 to 23
    timeStruct->tm_min,       // minutes, range 0 to 59
    timeStruct->tm_sec);      //update the ISO 8601 time strings

  printf("New File: %s\n",logFileName);
  seconds = 0; // reset seconds to enter loop without delay

  do{
    while(time(NULL)==seconds)  // if time did not increment
      Sleep(MODALO_SLEEP_MORE);      // wait a bit extra until next second

    // if seconds incremented (code below acts on 1 sec timer roughly - never double counts or under counts)
    seconds=time(NULL); // update time
    timeStruct = localtime(&seconds); // update time structure

    if((timeStruct->tm_min+(timeStruct->tm_hour*60)-config.startLog)%config.fileInterval == 0) { //file interval starts
      sprintf(logFileName,"../log/modalo_%04d_%02d_%02d_%02d_%02d_%02d.csv",
        timeStruct->tm_year+1900, // The number of years since 1900
        timeStruct->tm_mon+1,     // month, range 0 to 11
        timeStruct->tm_mday,      // day of the month, range 1 to 31
        timeStruct->tm_hour,      // hours, range 0 to 23
        timeStruct->tm_min,       // minutes, range 0 to 59
        timeStruct->tm_sec);      //update the ISO 8601 time strings

      printf("New File: %s\n",logFileName);
    } // file interval ends.

    if(seconds%config.sampleInterval == 0) { //sample interval starts
      if(!writeLogFile(logFileName,map,timeStruct)) {
        modaloPrintLastError();
      }
    } // sample interval ends.

    if(seconds%config.pollInterval == 0) { //poll interval starts
      if(!readReg(ctx,&map.reg[regIndex])) modaloPrintLastError();
      if(++regIndex==map.mapSize) regIndex=0; // reset index
    } // poll interval ends.

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
    else if(modbus_set_slave(ctx, 1) == -1)
    {
	     modaloSetLastError(EMODBUS_INIT,"Unable to set modbus slave in modbus context!");
	     modbus_free(ctx);
       ctx=NULL;
		}
		else if(modbus_connect(ctx) == -1 )
		{
	    sprintf(error,"Unable to connect to %s",config->port);
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

  if(reg->regSizeU16 == 2) // U32
    reg->value = (float)((reg->valueU16[0]*65536 + reg->valueU16[1]) * reg->multiplier)/ reg->divisor;
  else // U16
    reg->value = (float)(reg->valueU16[0] * reg->multiplier)/ reg->divisor;
  printf("REGNAME:%s, RAWVALUE:0x%04X%04X, VALUE:%0.2f\n",
    reg->regName,
    reg->valueU16[0],
    reg->valueU16[1],
    reg->value);
	fflush(stdout);
  return 1;
}

int writeLogFile(char* logFileName,MAP map,struct tm *timeStruct)
{
  FILE* logFileHandle=NULL;  // Handle for holding log file
  int count=0; // for counting the no. of registers written
  int fResult = 0; // for error checking fprintf

  logFileHandle = fopen(logFileName,"a"); // open log file in append mode
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
  fResult=fprintf(logFileHandle,"%04d-%02d-%02dT%02d:%02d:%02d+05:30",
    timeStruct->tm_year+1900, // The number of years since 1900
    timeStruct->tm_mon+1,     // month, range 0 to 11
    timeStruct->tm_mday,      // day of the month, range 1 to 31
    timeStruct->tm_hour,      // hours, range 0 to 23
    timeStruct->tm_min,       // minutes, range 0 to 59
    timeStruct->tm_sec);      // seconds, range 0 to 59
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
