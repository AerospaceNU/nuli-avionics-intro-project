#pragma once

#define RFM95_CS 8
#define RFM95_INT 3
#define RFM95_RST 4

#define RF95_FREQ 915.0

#define GPSMESSAGE_START_FLAG 0xB5
#define GPSMESSAGE_END_FLAG 0x62

struct __attribute__((packed)) GPSMessage {
	uint8_t startFlag = GPSMESSAGE_START_FLAG;
	uint8_t type = 1;
	uint8_t length = 20;
	int32_t latitude = 0;
	int32_t longitude = 0;
	int32_t satellites = 0;
	uint8_t crc = 0;
	uint8_t endFlag = GPSMESSAGE_END_FLAG;
};

uint8_t calcCRC(uint8_t const *data, uint8_t end);
