// #include <Arduino.h>
// #include <driver/spi_master.h>

// spi_device_handle_t spiDeviceHandleBMI160;
// spi_bus_config_t spiConfig;
// spi_device_interface_config_t spiDeviceBMI160;
// spi_transaction_t transaction;
// char output[9];

// void setup() {
//   Serial.begin(115200);
//   spiConfig.mosi_io_num = 23;
//   spiConfig.miso_io_num = 19;
//   spiConfig.sclk_io_num = 18;
//   spiConfig.quadhd_io_num = -1;
//   spiConfig.quadwp_io_num = -1;
//   spiConfig.max_transfer_sz = 32;

//   spiDeviceBMI160.clock_speed_hz = 10000000;
//   spiDeviceBMI160.spics_io_num = 5;
//   spiDeviceBMI160.address_bits = 8;
//   spiDeviceBMI160.command_bits = 0;
//   spiDeviceBMI160.mode = 0;
//   spiDeviceBMI160.queue_size = 7;
//   spiDeviceBMI160.input_delay_ns = 20;


//   transaction.addr = 0x00;
//   transaction.length = 16;
//   transaction.rxlength = 8;
//   transaction.flags = SPI_TRANS_USE_RXDATA;

//   spi_bus_initialize(VSPI_HOST, &spiConfig, 0);
//   spi_bus_add_device(VSPI_HOST, &spiDeviceBMI160, &spiDeviceHandleBMI160);
//   delay(100);
//   spi_transaction_t enableSPITransaction;
//   enableSPITransaction.addr = 0x7F;
//   enableSPITransaction.length = 16;
//   enableSPITransaction.rxlength = 8;
//   enableSPITransaction.flags = SPI_TRANS_USE_RXDATA;
//   spi_device_polling_transmit(spiDeviceHandleBMI160, &enableSPITransaction);
// }

// void loop() {
//   delay(1000);
//   Serial.println("available");

//   spi_device_polling_transmit(spiDeviceHandleBMI160, &transaction);
//   for (int byte = 0; byte < 4; byte++){
//     for (int i = 0; i < 7; i++){
//       bool x = transaction.rx_data[byte];
//       output[i] = ((x >> (7 - i)) & 1) ? '1' : '0';
//     }
//     Serial.print(String(output) + " ");
//   }
//   Serial.println();
// }


/*
 * Copyright (c) 2016 Intel Corporation.  All rights reserved.
 * See the bottom of this file for the license terms.
 */

/*
   This sketch example demonstrates how the BMI160 on the
   Intel(R) Curie(TM) module can be used to read gyroscope data
*/
      
#include <BMI160Gen.h>
unsigned long lastMicros = 0;
  int axRaw[1600], ayRaw[1600], azRaw[1600];

void setup() {
  Serial.begin(115200); // initialize Serial communication
  while (!Serial);    // wait for the serial port to open

  // initialize device
  Serial.println("Initializing IMU device...");
  BMI160.begin(BMI160GenClass::SPI_MODE, /* SS pin# = */5);
  uint8_t dev_id = BMI160.getDeviceID();
  Serial.print("DEVICE ID: ");
  Serial.println(dev_id, HEX);

  BMI160.setAccelerometerRange(2);
  BMI160.setAccelerometerRate(1600);
  Serial.println("Initializing IMU device...done.");
  delay(100);
}

float rawToAcc(int rawValue){
  return (float)rawValue / 32768 * 2;
}

void loop() {
  unsigned long start = micros();
  
  for (int i = 0; i < 1600; i++){
    BMI160.readAccelerometer(axRaw[i], ayRaw[i], azRaw[i]);
  }
  int delta = micros() - start;

  for (int i = 0; i < 1600; i++){
    Serial.print("g:\t");
    Serial.printf("%4.3f", rawToAcc(axRaw[i]));
    Serial.print("\t");
    Serial.printf("%4.3f", rawToAcc(ayRaw[i]));
    Serial.print("\t");
    Serial.printf("%4.3f", rawToAcc(azRaw[i]));
    Serial.println();
  }
  Serial.println(delta);
  Serial.println(String("Read with (Hz): ") + String(1600.0/(float) delta*1000000));
  Serial.println(String("One reading took (µs): ") + String((float) delta/1600));
  delay(10000);
}