# Modalo - Modbus Data Logger

A robust Windows-based Modbus data acquisition and logging system designed for industrial applications, with support for multiple devices, flexible configuration, and comprehensive error handling.

## Overview

Modalo is a professional-grade Modbus polling application that enables real-time data collection from multiple Modbus devices (RTU and TCP). It provides flexible configuration management, JSON-based register mapping, and detailed logging capabilities with advanced error handling.

## Features

### Core Functionality
- **Multi-Device Polling**: Poll and collect data from multiple Modbus devices simultaneously
- **Flexible Configuration**: Editable configuration files for easy portability and deployment
- **Register Support**: Full support for U16, U32, and IEEE-float32 register types
- **Endianness Control**: Support for both big-endian and little-endian register formats
- **JSON Configuration**: Parse register maps and settings from JSON files (beta)
- **Backward Compatibility**: Maintains compatibility with older configuration and map file formats

### Data Management
- **Plant Code Tracking**: Include plant identification in configuration and log filenames
- **UTC Time Format**: All timestamps use UTC format with Z terminator for standardized logging
- **Device Identification**: Uses Slave ID for unique device identification
- **Flexible Logging**: Output data to console or log files with configurable verbosity

### Reliability & Security
- **Error Handling**: Detailed error handling and reporting through dedicated error module
- **Security**: Preprocessor directives to hide non-API functions from DLL export (mitigates DLL injection attacks)
- **Verbose Mode**: Debug mode for troubleshooting and detailed operation tracking
- **Standard Windows Runtime**: Compiled with standard UCRT for maximum compatibility

### Platform Support
- **Windows 9+**: Requires Windows 9 or later
- **DLL Architecture**: Core Modbus functionality encapsulated in DLL for version control and stability

## System Requirements

- **OS**: Windows 9 or later
- **Runtime**: Standard UCRT (Universal C Runtime) library
- **Architecture**: x86/x64

## Installation

1. Download the latest release
2. Extract files to your desired installation directory
3. Configure the application using the config file (see Configuration section)
4. Run the executable

## Configuration

### Config File Structure
The application uses an editable configuration file to specify:
- Device connection parameters (IP, port, or serial port)
- Polling intervals
- Plant code identifier
- Register map file path
- Output log file path
- Verbosity settings

### Register Map (map.json)
Define your register mappings in JSON format:
```json
{
  "registers": [
    {
      "name": "voltage",
      "address": 100,
      "type": "IEEE-float32",
      "endian": "big"
    },
    {
      "name": "current",
      "address": 102,
      "type": "U16",
      "endian": "little"
    }
  ]
}
```

**Supported Register Types:**
- `U16`: 16-bit unsigned integer
- `U32`: 32-bit unsigned integer
- `IEEE-float32`: 32-bit floating point

## Usage

### Basic Operation
```
modalo.exe [config_file.ini]
```

### Output Files
- **Log Files**: Located in configured directory with naming convention: `PLANTCODE_YYYY-MM-DD.log`
- **Console Output**: Real-time polling status and error messages (if verbose mode enabled)

### Verbose Mode
Enable debug logging by setting verbose mode in configuration:
- Detailed function call tracing
- Register read/write debugging
- Connection status information

## Version History

### Current Version: V2.1
- Verbose mode with debug-level logging
- Float register type support (U32, U16, IEEE-float32)
- Backward compatible with previous config and map file formats

### Recent Updates (V2.0)
- Multi-device polling functionality
- Enhanced security with hidden DLL exports
- Slave ID-based device identification

### Upcoming Features (VX.X)
- GUI (WINAPI) for hiding console window
- UI editor for configuration and map files
- Windows 10/11 optimization

## Supported Devices

Modalo has been tested and verified with:
- Solis Inverters (V1.1+)
- Generic Modbus RTU devices
- Generic Modbus TCP devices

## Architecture

### Components
1. **Main Application**: Handles polling logic and file I/O
2. **libmodbus.dll**: Core Modbus protocol implementation with hidden non-API exports
3. **Configuration Parser**: Reads and validates config files
4. **JSON Parser**: Processes register map definitions
5. **Error Handler**: Centralized error logging and reporting

### DLL Design
- Minimized export surface to prevent injection attacks
- Version-controlled Modbus implementation
- Optimized for standard Windows runtime environments

## Troubleshooting

### Common Issues

**Connection Timeout**
- Verify device IP/port or serial port settings
- Check firewall settings for TCP connections
- Ensure Slave ID matches device configuration

**Register Read Errors**
- Verify register address matches device documentation
- Check endianness setting matches device format
- Confirm register type (U16/U32/IEEE-float32) is correct

**Incomplete Config File**
- Validate JSON syntax in configuration files
- Ensure all required fields are present
- Check file encoding (UTF-8 recommended)

**Parity Errors**
- For RTU connections, verify parity setting matches device
- Common settings: None, Even, or Odd
- Recompile from source if using non-standard parity modes

## Building from Source

The project includes source for:
- Main application (C/C++)
- libmodbus.dll wrapper (C)
- Configuration and JSON parsers

Build with Visual Studio 2019+ or compatible compiler with UCRT support.

## License

[Specify your license here]

## Contributing

Contributions are welcome! Please ensure compatibility with:
- Windows 9+
- Standard UCRT runtime
- Existing configuration file format (for backward compatibility)

## Support

For issues, questions, or feature requests, please:
1. Check the Troubleshooting section
2. Enable verbose mode for detailed debugging information
3. Review log files in the configured output directory
4. Contact the development team with:
   - Windows version
   - Device model and Modbus version
   - Configuration and map files (sanitized)
   - Relevant log excerpts

## Roadmap

- **Near Term**: UI-based configuration editor
- **Future**: Enhanced Modbus TCP security (Modbus/TCP over SSL)
- **Optimization**: Performance improvements for high-frequency polling

---

**Current Version**: 2.1  
**Last Updated**: May 5, 2026  
**Platform**: Windows 9+
