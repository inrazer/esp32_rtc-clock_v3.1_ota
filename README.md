# esp32_rtc-clock_v3.1_ota

This sketch is to drive the nixie display unit R 1250 M/C from ML (Mechanikai Laboratórium) Budapest.

*******************
* Background info *
*******************
The display unit was originally used as a digital frequency scale in a short wave wireless telegraphy receiver.
The devices were designed to spy wireless telegraphy in short wave band, in formerly GDR.

*******************
*   Sketch info   *
*******************
At start up the ESP32 connect to the defined WLAN, next it syncs the internal RTC with a defined NTP server.
Until the sync is complete, the step-up for the +160V anode supply stays disabled.
If the RTC is synced, the anode supply is enabled and all nixie cathodes are cycled 2 times.

After start-up the diplay show actual time until all times end ... or power gets interrupted.
