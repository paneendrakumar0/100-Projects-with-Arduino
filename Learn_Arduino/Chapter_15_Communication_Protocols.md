# Chapter 15: Communication Protocols (I2C, SPI, UART)

As you build more complex projects, you will buy sensors and screens that require more than just a simple `HIGH` or `LOW` signal. They need to send complex data, like text or precise GPS coordinates.

To do this, the Arduino and the sensor must speak the same "language." These languages are called **Communication Protocols**. 

There are three major protocols you will use constantly in the 100 Days of Arduino challenge:

## 1. UART (Serial Communication)
We already covered this in Chapter 8! This is how the Arduino talks to your computer via the USB cable. 
- **Pins Used:** `TX` (Transmit) and `RX` (Receive).
- **How it works:** It requires two wires. The `TX` pin of the sender connects to the `RX` pin of the receiver, and vice versa. 
- **The Catch:** It is point-to-point. You can only connect ONE device to the `TX`/`RX` line at a time. If you connect a GPS module to your Arduino's main hardware serial pins (Pins 0 and 1), you cannot use the Serial Monitor at the same time!
- **Common Modules:** GPS Modules (NEO-6M), Bluetooth Modules (HC-05).

## 2. I2C (Inter-Integrated Circuit)
I2C is the most popular protocol for modern sensors and displays because it only requires two wires, and you can connect dozens of devices to the exact same two wires!
- **Pins Used:** `SDA` (Data) and `SCL` (Clock). On the Uno, these are `A4` (SDA) and `A5` (SCL), or the dedicated pins near the USB port.
- **How it works:** Every I2C device has a unique "address" (like a house address). The Arduino acts as the "Master". It shouts down the wire: *"Hey, device at address 0x27, what is the temperature?"* Only the device with that address will reply.
- **Common Modules:** OLED Displays, 16x2 LCD screens (with I2C backpacks), MPU6050 Accelerometers.

## 3. SPI (Serial Peripheral Interface)
SPI is the fastest of the three protocols. It uses 4 wires. You use SPI when you need to transfer massive amounts of data very quickly, like writing a file to an SD card or displaying an image on a color TFT screen.
- **Pins Used:** `MOSI` (Master Out Slave In), `MISO` (Master In Slave Out), `SCK` (Clock), and `CS` (Chip Select). On the Uno, these are usually pins 11, 12, 13, and 10.
- **How it works:** Like I2C, you can connect multiple devices to the same SPI wires. But instead of addresses, the Arduino uses the `CS` (Chip Select) wire. Every device needs its own `CS` wire. To talk to a specific device, the Arduino pulls its `CS` wire LOW to "wake it up", talks to it at lightning speed, and then pulls it HIGH to put it back to sleep.
- **Common Modules:** SD Card Readers, nRF24L01 Wireless Transceivers, RFID Readers (MFRC522).

## Summary Cheatsheet
- Need to talk to your PC or a Bluetooth module? Use **UART**.
- Need to connect 5 different sensors using only 2 pins? Use **I2C**.
- Need to transfer large files or fast graphics? Use **SPI**.

---

[<-- Back to Main Guide](./README.md)
