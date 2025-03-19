# lvgl_demo

## Introduction
This project is a demo of the LVGL library, which is a lightweight graphics library for embedded systems. It provides a simple and efficient way to create graphical user interfaces (GUIs) for embedded systems. The library is designed to be easy to use and efficient, making it ideal for use in resource-constrained environments.

## Supported Platforms and Interfaces
- [] T3: SPI
- [x] T5AI: RGB/8080/SPI
- [ ] ESP32

## Supported Drivers
### Screen
- SPI
    - [x] ST7789 
    - [x] ILI9341
    - [x] ILIGC9A01

- RGB
    - [x] ST9488

### Touch
- I2C
    - [x] GT911
    - [x] CST816

### Rotary Encoder

> More driver adaptations and testing in progress...

## Usage Process
1. Run `tos menuconfig` to configure the project.

2. Configure the corresponding screen/touch/rotary encoder drivers.

3. Configure the corresponding GPIO pins.

4. Compile the project.

5. Burn and run.