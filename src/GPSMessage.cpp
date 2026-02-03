#include <Arduino.h>

#include "GPSMessage.h"

uint8_t calcCRC(uint8_t const *data, uint8_t len) {
	constexpr uint8_t polynomial = 0x19;
	uint8_t ret = 0;
	for (uint8_t i = 0; i < len; ++i) {
		ret ^= data[i];
		for (uint8_t b = 0; b < 8; ++b) {
			if (ret & 0x80)
				ret = (ret << 1) ^ polynomial;
			else
				ret <<= 1;
		}
	}
	return ret;
}
