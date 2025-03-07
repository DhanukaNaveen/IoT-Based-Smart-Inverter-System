#include <avr/io.h>      // AVR I/O library for register manipulation
#include <util/delay.h>  // Delay functions

// Define pins for sensors (using ADC pins)
#define VOLTAGE_SENSOR_PIN 0  // ADC0 (Pin 0) for voltage sensor
#define CURRENT_SENSOR_PIN 1  // ADC1 (Pin 1) for current sensor
#define RELAY_PIN PB5         // Pin 13 (PB5) for relay (Arduino Uno)

float voltageCalibrationFactor = 250.0;  // Calibration factor for voltage sensor
float currentCalibrationFactor = 1.82;   // Calibration factor for current sensor

// Variables to store sensor offsets
float voltageOffset = 0.0;
float currentOffset = 0.0;

// Threshold for voltage (in Volts)
#define VOLTAGE_THRESHOLD_HIGH 200.0

// Moving average window size
#define MOVING_AVERAGE_SIZE 10
float voltageReadings[MOVING_AVERAGE_SIZE];
float currentReadings[MOVING_AVERAGE_SIZE];
int voltageIndex = 0;
int currentIndex = 0;

// Function to initialize ADC
void initADC() {
    // Set reference voltage to AVCC and left adjust ADC result
    ADMUX = (1 << REFS0) | (1 << ADLAR);  
    // Enable ADC and ADC interrupt (not used in this case)
    ADCSRA |= (1 << ADEN); 
}

// Function to read ADC value
uint16_t readADC(uint8_t channel) {
    // Select the channel (0 for ADC0, 1 for ADC1, etc.)
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
    // Start the conversion
    ADCSRA |= (1 << ADSC);
    // Wait for the conversion to finish
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

// Function to initialize relay pin (Pin 13, PB5)
void initRelayPin() {
    DDRB |= (1 << RELAY_PIN);  // Set PB5 as output
}

// Function to read and calculate RMS voltage
float readVoltage() {
    const int numSamples = 1000;
    long sumSquares = 0;
    for (int i = 0; i < numSamples; i++) {
        uint16_t sensorValue = readADC(VOLTAGE_SENSOR_PIN);  // Read from voltage sensor (ADC0)
        int16_t voltageSample = sensorValue - voltageOffset;
        sumSquares += voltageSample * voltageSample;
        _delay_us(50);  // Delay between reads
    }
    long meanSquare = sumSquares / numSamples;
    float rms = sqrt(meanSquare);
    float adcVoltage = (rms / 1024.0) * 5.0;  // Convert ADC value to voltage (5V reference)
    float voltage = adcVoltage * voltageCalibrationFactor * sqrt(2);  // Multiply by sqrt(2) for peak voltage
    return voltage;
}

// Function to read and calculate RMS current
float readCurrent() {
    const int numSamples = 1000;
    long sumSquares = 0;
    for (int i = 0; i < numSamples; i++) {
        uint16_t sensorValue = readADC(CURRENT_SENSOR_PIN);  // Read from current sensor (ADC1)
        int16_t currentSample = sensorValue - currentOffset;
        sumSquares += currentSample * currentSample;
        _delay_us(50);  // Delay between reads
    }
    long meanSquare = sumSquares / numSamples;
    float rms = sqrt(meanSquare);
    float adcVoltage = (rms / 1024.0) * 5.0;  // Convert ADC value to voltage (5V reference)
    float current = adcVoltage / 0.1;  // ACS712 20A sensitivity is 100mV/A
    current *= currentCalibrationFactor * sqrt(2);  // Multiply by sqrt(2) for peak current
    return current;
}

// Function to calculate the average of the values in the array
float calculateAverage(float readings[]) {
    float sum = 0.0;
    for (int i = 0; i < MOVING_AVERAGE_SIZE; i++) {
        sum += readings[i];
    }
    return sum / MOVING_AVERAGE_SIZE;
}

// Main program loop
int main(void) {
    // Initialize ADC and relay pin
    initADC();
    initRelayPin();

    // Initialize readings arrays
    for (int i = 0; i < MOVING_AVERAGE_SIZE; i++) {
        voltageReadings[i] = 0;
        currentReadings[i] = 0;
    }

    // Main loop
    while (1) {
        // Read voltage and current
        float voltage = readVoltage();
        float current = readCurrent();

        // Add the new readings to the moving average
        voltageReadings[voltageIndex] = voltage;
        currentReadings[currentIndex] = current;

        // Increment the index and reset if necessary
        voltageIndex = (voltageIndex + 1) % MOVING_AVERAGE_SIZE;
        currentIndex = (currentIndex + 1) % MOVING_AVERAGE_SIZE;

        // Calculate the average value from the moving window
        float avgVoltage = calculateAverage(voltageReadings);
        float avgCurrent = calculateAverage(currentReadings);

        // Display readings via UART (optional for debugging)
        printf("Voltage: %.2f V\tCurrent: %.2f A\n", avgVoltage, avgCurrent);

        // Check if voltage exceeds the threshold
        if (avgVoltage > VOLTAGE_THRESHOLD_HIGH) {
            // Turn off the relay (bulb OFF) if the voltage > 200V
            PORTB &= ~(1 << RELAY_PIN);  // Turn off relay (low)
            printf("Bulb OFF: Voltage exceeds threshold\n");
        } else {
            // Turn on the relay (bulb ON) if the voltage is below threshold
            PORTB |= (1 << RELAY_PIN);   // Turn on relay (high)
            printf("Bulb ON: Normal conditions\n");
        }

        _delay_ms(1000);  // Wait for 1 second before the next reading
    }

    return 0;
}
