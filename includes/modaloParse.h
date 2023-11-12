#ifndef H_PARSE_MODALO
#define H_PARSE_MODALO

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _WIN32
  /* You should define MODALO_EXPORT *only* when building the DLL. */
  #ifdef MODALO_EXPORT
    #define MODALO_API __declspec(dllexport)
  #else
    #define MODALO_API __declspec(dllimport)
  #endif
  /* Define calling convention in one place, for convenience. */
  #define MODALO_CALL __stdcall
#else /* _WIN32 not defined. */
  /* Define with no value on non-Windows OSes. */
  #define MODALO_EXPORT
  #define MODALO_CALL
#endif

#define VALUE_SEPARATOR " \n"
#define PARAMETER_SEPARATOR "_"
#define MAXLINELENGTH 50
#define COMMENT_CHAR '#'
#define BUFFER_SIZE 1024         // 1kb Buffer
#define MAX_FILESIZE 1024*1024*2 // 2MB file
#define REGNAME_MAXSIZE 30
#define PARITY_ODD 'O'
#define PARITY_EVEN 'E'
#define PARITY_NONE 'N'
#define MAKE_MODEL_NAMESIZE 16
#define MAX_MODBUS_DEVICES 15
#define MAX_BAUD_RATE 199999
#define MAX_DATA_BITS 32
#define MIN_DATA_BITS 8
#define MAX_SAMPLE_INTERVAL 86400
#define MAX_FILE_INTERVAL 1440
#define FILEPATHSIZE 128

// structure definitions for holding register information
typedef struct REG {
  char regName[REGNAME_MAXSIZE];
  uint16_t regAddress;
  uint16_t regSizeU16;
  uint16_t byteReversed;
  uint16_t bitReversed;
  uint16_t functionCode;
  uint16_t multiplier;
  uint16_t divisor;
  uint16_t movingAvgFilter;
  uint16_t valueU16[2];
  float value;
}REG;

// structure definitions for holding map information
typedef struct MAP {
  REG* reg;              //pointer to dynamically allocated reg array
  size_t mapSize;        //size of the reg array
  unsigned int regIndex; //variable to itterate over reg if needed.
}MAP;

// structure for holding DEVICE specific informations
typedef struct DEVICE {
  char make[MAKE_MODEL_NAMESIZE];
  char model[MAKE_MODEL_NAMESIZE];  
  char assetID[MAKE_MODEL_NAMESIZE];
  unsigned int capacity;
  unsigned int plantCode;
  unsigned int slaveID;
  MAP map;
}DEVICE;

// structure definitions for holding configuration information
typedef struct CONFIG{
  // modbus configurations
  char port[6];
  unsigned int baud;
  char parity;
  unsigned int dataBits;
  unsigned int stopBits;
  
  // sampling and data logging configurations
  unsigned int sampleInterval;
  unsigned int fileInterval;
  unsigned int pollInterval;
  unsigned int startLog; // in minutes from 00:00
  unsigned int stopLog; // in minutes from 00:00
  char logFilePath[FILEPATHSIZE];

  // DEVICE specific configurations
  DEVICE device[MAX_MODBUS_DEVICES];
}CONFIG;

//macro definitions
_MODALO_forEachDevice(i) for(int i=0;i<MAX_MODBUS_DEVICES;i++)

//API functions
MODALO_API int MODALO_CALL parseModaloConfigFile(CONFIG* config, char * FileName);
MODALO_API void MODALO_CALL freeMAP(MAP map);
MODALO_API MAP MODALO_CALL parseModaloJSONFile(char* fileName, char* modelName); // if u use this function, then u should free the memory using freeMAP()
MODALO_API void MODALO_CALL printModaloConfig(CONFIG config);
MODALO_API void MODALO_CALL printModaloMap(MAP map);

// non API functions (not accessible by including header and linking library)
int validateModaloToken(CONFIG *config, char *parameter, char *value); // validates and saves parameter and value
int parseModaloConfigString(CONFIG* config, char * line); // parses string into parameter and token
char* readFileToBuffer(char * fileName);
int validateModaloIntegerString(char * valString, int lLimit, int uLimit);
int validateModaloDeviceToken(CONFIG* config, char * indexParameter, char * childParameter, char * value);
void cleanModaloConfigStruct(CONFIG *config);
int validateModaloCOMPORTString(char* value);
int validateModaloHHMMString(char* value);
int validateModaloFilePathString(char* value);

#ifdef __cplusplus
}
#endif

#endif
