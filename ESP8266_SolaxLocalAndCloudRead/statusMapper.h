#ifndef STATUS_MAPPER_H
#define STATUS_MAPPER_H

#include <Arduino.h>

// Map inverter status code to human-readable text
// Based on SolaX Cloud API documentation Appendix 8.1
String mapInverterStatus(String statusCode) {
  int code = statusCode.toInt();
  
  switch(code) {
    case 100: return "Waiting";
    case 101: return "Self-test";
    case 102: return "Normal";
    case 103: return "Fault";
    case 104: return "Perm Fault";
    case 105: return "Upgrade";
    case 106: return "EPS Detect";
    case 107: return "Off-grid";
    case 108: return "Self-test IT";
    case 109: return "Sleep";
    case 110: return "Standby";
    case 111: return "PV Wake Bat";
    case 112: return "Gen Detect";
    case 113: return "Generator";
    case 114: return "Fast Shtdwn";
    case 130: return "VPP";
    case 131: return "TOU-Self";
    case 132: return "TOU-Charge";
    case 133: return "TOU-Dschrg";
    default: return "Unknown";
  }
}

#endif

