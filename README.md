# rfid-tracking · RFID Reader (ESP32)

RFID reader firmware for **rfid-tracking**, a prototype for inventory management and product tracking with RFID, built as my undergraduate thesis in Information Systems at **PUCRS (2024)**.

| Repository | Role |
| --- | --- |
| [rfid-tracking](https://github.com/JoseMD12/rfid-tracking) | Parent repository: project overview, architecture and how to run everything |
| [tcc-back](https://github.com/JoseMD12/tcc-back) | REST API + WebSocket, business rules and persistence |
| [tcc-front](https://github.com/JoseMD12/tcc-front) | Web dashboard (React) |
| **ESP32-ReadTag** (this one) | RFID reader firmware (ESP32 + MFRC522) |

Each reader is installed at a **deposit** (a point in the supply chain). When a tag passes by, the ESP32 reads its content and notifies the API, which records the instance's movement to that deposit.

## Hardware

- **ESP32** (Wi-Fi microcontroller)
- **MFRC522** (13.56 MHz RFID reader/writer)
- **MIFARE Classic** tags (cards or key fobs)
- Breadboard and jumper wires

### Wiring ESP32 → MFRC522

| ESP32 | MFRC522 |
| --- | --- |
| 3.3V | 3.3V |
| GPIO 22 | RST |
| GND | GND |
| (not used) | IRQ |
| GPIO 19 | MISO |
| GPIO 23 | MOSI |
| GPIO 18 | SCK |
| GPIO 5 | SDA (SS) |

> Power the MFRC522 with **3.3V**. Connecting it to 5V can damage the module.

## How it works

1. On boot, the ESP32 connects to Wi-Fi and calls `GET /rfid` on the API to test the connection.
2. In the `loop`, it waits for a tag and reads **blocks 1 and 2** of the MIFARE card (default key `FF FF FF FF FF FF`).
3. The content written to the tag follows the `instanceId-productId` format, generated when instances are registered in the system.
4. It sends the read to the API:

```http
POST /product/event
Content-Type: application/json

{
  "actualDepositId": "<id of the deposit where this reader is installed>",
  "tagList": ["<instanceId>-<productId>"]
}
```

5. The API records the event and notifies the dashboard over WebSocket.

## Configuration

The firmware was written for the **Arduino IDE** with Espressif's ESP32 board package and the **MFRC522** library (by Miguel Balboa). The `WiFi` and `HTTPClient` libraries ship with the ESP32 package.

Before flashing, set the constants at the top of `communication.c`:

| Constant | Value |
| --- | --- |
| `ssidMovel` / `passwordMovel` | Wi-Fi network for the reader |
| `serverUrlMovel` | `http://<API machine IP>:3000/product/event` |
| `connectionTesteMovel` | `http://<API machine IP>:3000/rfid` |
| `depositId` | Deposit ID, registered beforehand in the system |

> The file is an Arduino sketch (C++). To open it in the Arduino IDE, save a copy as `communication.ino` inside a `communication/` folder.

The serial monitor (9600 baud) shows the card UID, the data read and the API response.

## Limitations

- The MFRC522 reads within a few centimeters and was used to **simulate** a long-range antenna. A real logistics scenario would call for UHF readers.
- Each reader sends one tag at a time, and the deposit is fixed in the firmware.
- Communication is plain HTTP without authentication, and the network and server are configured directly in the code.
