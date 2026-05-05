Modular Satellite Telemetry Transmitter

A ground-based prototype of a satellite telemetry transmitter system designed for efficient sensor data acquisition, adaptive sampling, buffering, and wireless transmission.

Overview

This project simulates a satellite transmitter system that:

* Collects sensor data
* Stores it during non-contact periods
* Transmits it in bursts (store-and-forward model)

Objectives

* Adaptive sensor sampling
* Timestamped data acquisition
* Circular buffer implementation
* Structured packetization with CRC
* Scalable modular design

---

Hardware Used

* Teensy 4.1
* GUVA S12SD UV Sensor
* BME280 Sensor
* LoRa SX1278 Module
* 433 MHz Antenna

Software Used

* Arduino IDE (Teensyduino)
* MATLAB

Key Features

* Adaptive sampling
* Timestamped data logging
* Circular buffering
* Store-and-dump transmission
* Packetization with CRC

Packet Structure

Version | ID | Sequence | Length | Timestamp | Payload | CRC

System Flow

Sensor → ADC → Buffer → Packet → Transmission → Ground Station

Results

* Real-time UV data acquisition
* Adaptive sampling validated
* Circular buffer tested
* Simulation completed

Team Members

* Gouri Saji
* Thomas T Roy
* Jefin Jaison
* Nivedya Pavithran

Future Work

* RF transmission testing
* Ground station validation
* Full satellite integration

License

MIT License

