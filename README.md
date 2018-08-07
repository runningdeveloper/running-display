# IOT Running Display

## Background

I enjoy running but have been struggling with consistency lately. Sometimes I just cannot get out of the door for even a short run.
I wanted to make a running display that would show me how I was doing during the week. So I made this IOT connected LED display.
It shows the following:

- The current day
- If I ran for more than 30min it will display a green dot
- If I didn’t run that day if will show a red dot

The idea is that I will have this hanging in my house and it will be in my face all the time and even my family members will see it. This should motivate me to try get more green dots than red.

## How it works

Consists of:

- 18 neo pixel LEDs
- 1 button
- D1 mini esp8266 board
- 3d printed box

I am using Azure's IOT hub to communicate with my device through the esp board. I am using three azure functions to do the following:

- A http function that does an api call to Strava (the app I use to record my running) to get my weeks running stats. Then it converts this into an array of LED values. Which it sends to an IOT hub topic. The esp is listening for this topic and then displays the result.
- Timer function is used to run every hour. This triggers the http function so I get new results to the topic every hour.
- A IOT event trigger function is also used. This is listening to a topic that gets fired from the display when the button is pushed. It will then like the timer call the http function so that new results are published. I added this because you may be back from a run and want to see the results immediatly and not wait for the timer to run.

## Video

[youtube video](https://youtu.be/zieGFv2LD78)

### Code

I have 3 directories. 

- 3dprint (stl files for the print)
- esp (the code running on the d1 mini, used platformio in vs code)
- functions (three azure functions, I used the online editor mostly to write these, so this is a copy)

I will write a more in-depth blog post about the challenges and learnings from this project.

For now though, if you would like run the code or understand it a bit better (rough info):

I setup a device in the IOT hub. Tip - [IOT hub explorer](https://github.com/Azure/iothub-explorer) helped for testing messages.

I used the the following environment variables for the azure functions (add in the app settings on the portal):

- AUTH_CODE, used so that from the timer and button code I can call the http function for strava securely
- IOTCONNECTION, IOT hub connection string
- STRAVA_CLIENT, STRAVA_SECRET, STRAVA_TOKEN from setting up a strava api application

I wanted to use async and await in the functions so had to setup the functions to use the beta version of azure functions (change in app settings to increase node version). Unfortunately the IOT hub connection didn't work in the beta (unless I missed something obious) so had to move the IOT button function to the stable version of the azure functions. That's why the button code has a promise compared to the timer function.

For the esp I used [platformio](https://platformio.org/) to write the code in vscode. Its quite nice. My c++ code is not great so tell me if you see some errors. Also if you want to run the esp code, you need to add some config variables in config_sample.h an rename it config.h

## Improvements

- Add an off period at night
- Better clasp to hold the device together and wall mounting system 
- Darker plastic so LED light doesn't bleed
- More LEDS for the Running text
- Send up a health check and notify me if it hasn't connected in a while
- Use the device twin to keep status or log so if it gets powered off I can get the desired state without calling strava api each time

## Acknowledgments

- [Neopixel library](https://github.com/adafruit/Adafruit_NeoPixel/blob/master/examples/simple/simple.ino)
- [Azure esp sample](https://github.com/Azure-Samples/iot-hub-feather-huzzah-client-app)
