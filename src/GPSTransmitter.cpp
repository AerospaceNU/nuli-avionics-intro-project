#include <Adafruit_GPS.h>
#include <Arduino.h>
#include <RH_RF95.h>
#include <SPI.h>

#include "GPSMessage.h"

RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Unnecessary but makes CCLS warnings go away (probably a CCLS issue)
auto const GPSSerial = dynamic_cast<HardwareSerial *>(&Serial1);

Adafruit_GPS gps(GPSSerial);

void setup() {
	pinMode(RFM95_RST, OUTPUT);
	digitalWrite(RFM95_RST, HIGH);

	gps.begin(9600);

	gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
	gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
	gps.sendCommand(PGCMD_ANTENNA);

	delay(1000);

	GPSSerial->println(PMTK_Q_RELEASE);

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
	(void)gps.read();

	if (!gps.newNMEAreceived())
		return;
	if (!gps.parse(gps.lastNMEA())) {
		Serial.println("Failed to parse NMEA!");
		return;
	}
	if (!gps.fix) {
		Serial.println("No fix");
		return;
	}

	struct GPSMessage msg;
	msg.latitude = gps.latitude_fixed;
	msg.longitude = gps.longitude_fixed;
	msg.satellites = gps.satellites;

	auto data = reinterpret_cast<uint8_t *>(&msg);

	msg.crc = calcCRC(data, offsetof(GPSMessage, crc));

	if (rf95.send(data, sizeof(msg))) {
		rf95.waitPacketSent();
		Serial.println("Sent packet...");
	} else {
		Serial.println("Send failed");
	}
}
