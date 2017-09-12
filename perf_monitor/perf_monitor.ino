#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>

#define BUFSIZE 32

struct MyColor
{
	int red;
	int green;
	int blue;
};

const int numLeds = 64;
const int pin = D6;

const char* ssid     = "Bluffs_Resident";
const char* password = "bluffsresident";

struct MyColor colors[numLeds];

int cycle = 0;

WiFiServer server(80);
WiFiClient client;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(numLeds, pin, NEO_GRB + NEO_KHZ800);

void setup()
{
	Serial.begin(9600);
	WiFi.begin(ssid, password);

	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	Serial.println(WiFi.localIP());
	Serial.println(WiFi.macAddress());

	server.begin();
	Serial.println("Server started");

	pinMode(LED_BUILTIN, OUTPUT);
	strip.begin();
	strip.show();
	strip.setBrightness(1);
}

void get_values(char *buf, int& cpu, int& mem, int& swap)
{
	int i;
	int j = 0;
	char tmp[8] = {0};

	for (i = 1; i < strlen((char*)buf); i++)
	{
		if (buf[i] == ',')
		{
			cpu = atof(tmp);
			++i;
			break;
		}
		tmp[j++] = buf[i];
	}
	for (j = 0; i < strlen((char*)buf); i++)
	{
		if (buf[i] == ',')
		{
			mem = atof(tmp);
			++i;
			break;
		}
		tmp[j++] = buf[i];

	}
	for (j = 0; i < strlen((char*)buf); i++)
	{
		if (buf[i] == '>')
		{
			swap = atof(tmp);
			break;
		}
		tmp[j++] = buf[i];

	}
}

void set_colors(char *buf)
{
	int i = 0;
	int cpu, mem, swap;

	get_values(buf, cpu, mem, swap);

	// set the LED colors, blinking the last one every other time
	for (i = 0; i < numLeds; i++)
	{
		colors[i].red = 0;
		colors[i].blue = 0;
		colors[i].green = 0;

		if ((cpu * (numLeds / 100.0) > i)
		    && ((cpu * (numLeds / 100.0) > i + 1) || !cycle))
		{
			colors[i].blue = 255;
		}
		if ((mem * (numLeds / 100.0) > i)
		    && ((mem * (numLeds / 100.0) > i + 1) || !cycle))
		{
			colors[i].green = 255;
		}
		if ((swap * (numLeds / 100.0) > i)
		    && ((swap * (numLeds / 100.0) > i + 1) || !cycle))
		{
			colors[i].red = 255;
		}
		strip.setPixelColor(i, colors[i].red, colors[i].green, colors[i].blue);
	}

	strip.show();

	cycle = (cycle + 1) % 2;
}

void loop()
{
	char buf[BUFSIZE] = {0};

	if (!client.connect("172.20.2.57", 2001))
	{
		Serial.println("Not connected");
		return;
	}

	Serial.println("Connected");
	while(1)
	{
		while (1)
		{
			int read = client.read((unsigned char*) buf,
					       BUFSIZE);
			if (read > 0)
			{
				Serial.print("READ: ");
				Serial.println(read);
				break;
			}
			delay(25);
		}

		Serial.println((char*)buf);
		set_colors(buf);
	}
}
