#include <Arduino.h>
#include <RH_RF95.h>
#include <SPI.h>

#include "GPSMessage.h"

RH_RF95 rf95(RFM95_CS, RFM95_INT);

uint16_t packet_num = 0;

void setup() {
	pinMode(RFM95_RST, OUTPUT);
	digitalWrite(RFM95_RST, HIGH);

	Serial.begin(115200);
	while (!Serial)
		delay(1);
	delay(100);

	Serial.println("Feather LoRa TX Test!");

	// manual reset
	digitalWrite(RFM95_RST, LOW);
	delay(10);
	digitalWrite(RFM95_RST, HIGH);
	delay(10);

	while (!rf95.init()) {
		Serial.println("LoRa radio init failed");
		while (1)
			;
	}
	Serial.println("LoRa radio init OK!");

	// Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
	if (!rf95.setFrequency(RF95_FREQ)) {
		Serial.println("setFrequency failed");
		while (1)
			;
	}
	Serial.print("Set Freq to: ");
	Serial.println(RF95_FREQ);

	// Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf =
	// 128chips/symbol, CRC on

	// The default transmitter power is 13dBm, using PA_BOOST.
	// If you are using RFM95/96/97/98 modules which uses the PA_BOOST
	// transmitter pin, then you can set transmitter powers from 5 to 23 dBm:
	rf95.setTxPower(23, false);
}

void loop() {
	if (!rf95.available())
		return;

	uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
	uint8_t len = sizeof(buf);

	if (!rf95.recv(buf, &len)) {
		Serial.println("Receive failed");
		return;
	}

	Serial.printf("Received %d bytes\n", len);

	if (len < sizeof(GPSMessage))
		return;

	uint8_t *start = buf;
	for (;;) {
		auto newStart = reinterpret_cast<uint8_t *>(
			memchr(start, GPSMESSAGE_START_FLAG, len));
		if (!newStart)
			return;
		uint8_t *endPos = newStart + offsetof(GPSMessage, endFlag);
		if (endPos < buf || endPos >= start + len)
			return;
		if (*endPos == GPSMESSAGE_END_FLAG) {
			start = newStart;
			break;
		}
		len -= newStart - start + 1;
		start = newStart + 1;
	}
	// HACK: terrible
	auto msg = reinterpret_cast<GPSMessage *>(start);
	if (calcCRC(start, offsetof(GPSMessage, crc)) != msg->crc) {
		Serial.printf("Failed CRC: %u\n", msg->crc);
	}

	constexpr int32_t DIV = 10000000;
	int32_t lat, latd, lon, lond;

	switch (msg->type) {
	case 1:
		lat = msg->latitude / DIV;
		latd = abs(msg->latitude) % DIV;
		lon = msg->longitude / DIV;
		lond = abs(msg->longitude) % DIV;
		Serial.printf("Packet %u: (latitude %d.%07d) (longitude %d.%07d) "
		              "(satellites %d)\n",
		              packet_num++, lat, latd, lon, lond, msg->satellites);
		Serial.printf(
			"https://maps.google.com/maps?z=12&t=m&q=loc:%d.%07d+%d.%07d\n",
			lat, latd, lon, lond);
		break;
	default:
		Serial.printf("Unexpected message type: %u (length %u)", msg->type,
		              msg->length);
	}
}
