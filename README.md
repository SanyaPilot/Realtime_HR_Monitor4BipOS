# Realtime_HR_Monitor4BipOS
App written on C for Amazfit Bip running BipOS 0.5.2
## Description:
This app can measure heart rate and draw chart on a screen in real time.

#### There are several settings:  
- Width in pixels for one measurement in the chart  
- Delay between measurements in range 0 - 60 seconds  
- Total time of measuring in range 0 - 60 minutes  

Since version 2.0 settings are saved into non-volatile memory. Swipe down to enter settings menu.

Measurements summary is displayed after time or manual stop.  
#### It consists of:
- Minimal heart rate
- Maximum heart rate
- Average heart rate

## Installing:  
Add the .elf file to the resources file with ResPack (more information on myamazfit.ru) and flash it on your watch
