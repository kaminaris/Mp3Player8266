#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

#include <random>

#include "AudioFileSourceID3.h"
#include "AudioFileSourceSD.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputSPDIF.h"

// To run, set your ESP8266 build to 160MHz, and include a SPIFFS partition
// big enough to hold your MP3 file. Find suitable MP3 file from i.e.
// https://docs.espressif.com/projects/esp-adf/en/latest/design-guide/audio-samples.html
// and download it into 'data' directory. Use the "Tools->ESP8266/ESP32 Sketch
// Data Upload" menu to write the MP3 to SPIFFS. Then upload the sketch
// normally.

AudioFileSourceSD* file;
AudioFileSourceID3* id3;
AudioOutputSPDIF* out;
AudioGeneratorMP3* mp3;

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void* cbData, const char* type, bool isUnicode, const char* string) {
	(void)cbData;
	Serial.printf("ID3 callback for: %s = '", type);

	if (isUnicode) {
		string += 2;
	}

	while (*string) {
		char a = *(string++);
		if (isUnicode) {
			string++;
		}
		Serial.printf("%c", a);
	}
	Serial.printf("'\n");
	Serial.flush();
}

auto songList = new std::vector<String>();
auto listenedSongs = new std::vector<String>();

void play(const char* phrase) {
	Serial.printf("Playing\n");
	out = new AudioOutputSPDIF(D0);
	file = new AudioFileSourceSD(phrase);
	id3 = new AudioFileSourceID3(file);
	id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
	if (!mp3->begin(file, out)) {
		Serial.println("Unable to play");
	}
}

void setup() {
	Serial.begin(115200);
	delay(1000);
	Serial.println();
	audioLogger = &Serial;

	Serial.print("Initializing SD card...");

	if (!SD.begin(15)) {
		Serial.println("initialization failed!");
		return;
	}
	Serial.println("initialization done.");


	mp3 = new AudioGeneratorMP3();

	mp3->begin(nullptr, out);

	auto root = SD.open("/");
	while (true) {
		auto f = root.openNextFile();
		if (!f) {
			// no more files
			break;
		}

		auto name = String(f.name());
		// name.toLowerCase();
		if (name.endsWith(".mp3")) {
			songList->push_back(name);
		}
	}
	std::random_shuffle(songList->begin(), songList->end());

	for (auto& song : *songList) {
		Serial.println(song);
	}
}

void loop() {
	if (mp3->isRunning()) {
		if (!mp3->loop()) {
			mp3->stop();
			file->close();
			id3->close();
			out->stop();
			Serial.println("MP3 done");
		}
	}
	else {
		Serial.println("MP3 next song");
		delay(1000);

		if (songList->empty()) {
			for (auto& song : *listenedSongs) {
				songList->push_back(song);
			}
			listenedSongs->clear();
			std::random_shuffle(songList->begin(), songList->end());
		}

		auto fName = songList->back();
		songList->pop_back();
		listenedSongs->push_back(fName);

		play(fName.c_str());
		Serial.printf("Playback of '%s' begins...\n", fName.c_str());

		delay(200);
	}
}