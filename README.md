# Gyro SPI with STM32F427VIT6
## Documentation Information
1. Doxygen generated HTML files are hosted at: [karthikdani.github.io/Gyro_SPI_STM32F427VIT6/](https://karthikdani.github.io/Gyro_SPI_STM32F427VIT6/)
2. PDF Document consisting of all the functions, (generated with Doxygen LaTeX files) produced LaTeX, is at: [docs/latex/refman.pdf](docs/latex/refman.pdf)

## Files 
1. Source code Directory: [Core/Src](Core/Src/)
2. Headers: [Core/Inc](Core/Inc/)

## Main Code Brief: 
The outline is firmware solution written by me, developed on top of [ST's Drivers for I3G4250D](https://github.com/STMicroelectronics/i3g4250d-pid) for STM32F427VIT6 that interacts with **I3G4250D gyroscope sensor** over **SPI** and sends the angular velocity data through **UART**.:

### 1. **Initialization**
   - **SPI1 and UART Setup:**  
     The SPI interface (`MX_SPI1_Init`) and the UART interface (`MX_USART2_UART_Init`) are initialized to communicate with the gyroscope and transmit data. SPI is configured in master mode, and UART is configured for 115200 baud rate communication. The prescaler is set to 64 and supports ~1.4MBits per Second.

   - **GPIO Setup:**  
     Port A GPIO pin 4 is `GYRO_CS_OUT_Pin` of the MCU is configured as an output to manage SPI's Chip Select. First set to `LOW` and then to `HIGH`. (Pulling it always high would result in I2C protocol from the sensor hardware).

   - **Sensor Initialization:**
     The `dev_ctx` struct is used to hold the function pointers for reading and writing to the sensor, and the device's boot time is accounted for by calling `power_on_delay(BOOT_TIME)`.

### 2. **Gyroscope Configuration**
   - **Sensor Power-On Delay:**  
     After initialization, the `BOOT_TIME` constant sets a delay to allow the sensor to power up.
   
   - **Device Check:**  
     The code verifies that the sensor's ID matches `I3G4250D_ID` using `i3g4250d_device_id_get`. If the device is not found, the program enters an infinite loop. This exception can be managed as per our needs 

   - **Gyroscope Settings:**  
     The gyroscope is configured with high-pass filtering (`i3g4250d_filter_path_set`), data rate (`i3g4250d_data_rate_set`), and bandwidth settings. These settings control the frequency range and noise filtering of the sensor's output.

### 3. **Data Collection Loop**
   - **Reading Angular Rate Data:**
     The code checks if new data is available (`i3g4250d_flag_data_ready_get`). If new data is available, it reads the raw angular velocity values in 3 axes (X, Y, Z) using `i3g4250d_angular_rate_raw_get`.

   - **Conversion to mDPS:**  
     The raw angular rates are converted from the sensor's raw data format (dps) to millidegrees per second (mDPS) using the `i3g4250d_from_fs245dps_to_mdps` function.

   - **Data Transmission via UART:**  
     The angular velocity data is formatted into a string and transmitted via UART using `HAL_UART_Transmit`. This allows the microcontroller to send data to a connected terminal or computer.

### 4. **SPI Read and Write Functions**
   - **`write_to_i3g4250d`:**  
     This function writes data to a specified register of the gyroscope. It sends the register address (with the read-write bit cleared) followed by the data to be written. It uses the `HAL_SPI_Transmit` function to communicate with the sensor.

   - **`read_from_i3g4250d`:**  
     This function reads data from the gyroscope by first sending the register address (with the read-write bit set) and then receiving the data. The `HAL_SPI_TransmitReceive` function is used for this two-way communication.

Made with ❤️ for Embedded Systems!