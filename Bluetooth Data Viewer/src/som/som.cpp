#include "som.h"

SystemOnChip esp;
Terminal terminal;
EMMC_Memory emmc;

BluetoothLowEnergyServer bleServer;
RealTimeClock rtc;
SX1509 io_expansion;
AD7689 adc;
ACAN2517FD can(CAN0_CONTROLLER_CS_PIN, esp.hspi, CAN0_CONTROLLER_INT_PIN);
Adafruit_NeoPixel pixels(2, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void IRAM_ATTR updateSystemTime()
{
    rtc.updateMillisecondsCounter(SoM.system_time);
}

void SystemOnModule::onBootError()
{
    SoM.initLED();
    pixels.setPixelColor(0, pixels.Color(100, 0, 0));
    pixels.show();
    terminal.println("\n\nStopping boot");
    while (1)
        ;
}

void SystemOnModule::initAll(bool debug_enabled)
{
    long initial_time = millis();

    if (debug_enabled)
        debugging_enabled = true;

    if (!initIOExpansion())
        onBootError();

    if (!initRTC())
        onBootError();

    if (!initADC())
        onBootError();

    if (!initCAN())
        onBootError();

    if (!initEMMC())
        onBootError();

    if (!initBLE())
        onBootError();

    terminal.setTimeKeeper(system_time);

    esp.timer0.setup();
    esp.timer0.attachInterrupt(updateSystemTime);
    esp.timer0.timerPeriodMilliseconds(1);
    esp.timer0.enableInterrupt();

    initLED();

    terminal.printMessage(TerminalMessage("Boot time: " + String(millis() - initial_time) + " mS", "SOM", INFO, micros()));
}

bool SystemOnModule::initIOExpansion()
{
    // 1. Begin I2C bus
    esp.i2c0.begin(I2C0_SDA_PIN, I2C0_SCL_PIN, I2C0_CLK_FREQUENCY); // Refer to soc_example.cpp for information on this function

    // 2. Begin IO expansion SX1509
    ESP_ERROR initialize_io_expansion = io_expansion.begin(&esp.i2c0, IO_EXP_ADDRESS);

    if (initialize_io_expansion.on_error) // Catch error
    {
        terminal.printMessage(TerminalMessage(initialize_io_expansion.debug_message, "I/O", ERROR, micros()));
        return false;
    }

    io_expansion.pinMode(V3V3_OUT_PIN_A, OUTPUT);
    io_expansion.digitalWrite(V3V3_OUT_PIN_A, HIGH);
    io_expansion.pinMode(V3V3_OUT_PIN_B, OUTPUT);
    io_expansion.digitalWrite(V3V3_OUT_PIN_B, HIGH); // 3.3V Out

    // 3. Setup GPIOs
    io_expansion.pinMode(V5V_EN_PIN, OUTPUT);
    io_expansion.digitalWrite(V5V_EN_PIN, HIGH); // 5V Output

    if (debugging_enabled)
    {
        io_expansion.pinMode(V3V3_DEBUG_PIN, OUTPUT);
        io_expansion.pinMode(GND_DEBUG_PIN, OUTPUT);
        io_expansion.digitalWrite(GND_DEBUG_PIN, LOW);   // GND Debug
        io_expansion.digitalWrite(V3V3_DEBUG_PIN, HIGH); // 3.3V Debug
    }

    io_expansion.pinMode(ADC0_EN_PIN, OUTPUT);
    io_expansion.digitalWrite(ADC0_EN_PIN, HIGH); // ADC

    io_expansion.pinMode(CAN0_EN_PIN, OUTPUT);
    io_expansion.digitalWrite(CAN0_EN_PIN, LOW); // CAN

    terminal.printMessage(TerminalMessage("All systems enabled", "I/O", INFO, micros()));

    return true;
}

bool SystemOnModule::initRTC()
{
    long initial_time = micros();

    ESP_ERROR initialize_rtc = rtc.begin(RealTimeClock::RV3028_IC, esp.i2c0);

    if (initialize_rtc.on_error)
    {
        terminal.printMessage(TerminalMessage(initialize_rtc.debug_message, "RTC", ERROR, micros()));
        return false;
    }

    if (debugging_enabled)
    {
        terminal.printMessage(TerminalMessage("RTC initialized", "RTC", INFO, micros(),
                                              micros() - initial_time));
    }

    // rtc.setToCompilerTime();

    system_time = rtc.getTime();

    return true;
}

bool SystemOnModule::initADC()
{
    long initial_time = micros();

    // 1. Begin SPI bus
    esp.vspi.begin(VSPI_CLCK_PIN, VSPI_MISO_PIN, VSPI_MOSI_PIN);
    esp.vspi.setFrequency(VSPI_CLCK_FREQUENCY);

    // 2. Begin ADC
    adc.begin(ADC0_CS, esp.vspi, VSPI_CLCK_FREQUENCY);
    // adc.enableFiltering(true);

    // 3. Run self test
    if (adc.selftest() == false)
    {
        terminal.printMessage(TerminalMessage("Error initializing ADC", "ADC", ERROR, micros()));
        return false;
    }

    if (debugging_enabled)
    {
        terminal.printMessage(TerminalMessage("ADC initialized", "ADC", INFO, micros(),
                                              micros() - initial_time));
    }

    return true;
}

bool SystemOnModule::initCAN()
{
    long initial_time = micros();

    // 1. Init SPI bus
    esp.hspi.begin(HSPI_CLCK_PIN, HSPI_MISO_PIN, HSPI_MOSI_PIN);

    // 2. Set CAN settings
    ACAN2517FDSettings settings(ACAN2517FDSettings::OSC_40MHz, 1000 * 1000, DataBitRateFactor::x1);
    settings.mDriverReceiveFIFOSize = 200;

    // 3. Attempt to initialize
    const uint32_t error_code = can.begin(settings, []
                                          { can.isr(); });

    if (error_code != 0)
    {
        terminal.printMessage(TerminalMessage("Could not initialize CAN controller. Error code: " + String(error_code),
                                              "CAN", ERROR, micros()));
        return false;
    }

    if (debugging_enabled)
    {
        terminal.printMessage(TerminalMessage("CAN controller initialized", "CAN", INFO, micros(),
                                              micros() - initial_time));
    }

    return true;
}

bool SystemOnModule::initEMMC()
{
    long initial_time = micros();
    ESP_ERROR initialize_emmc = emmc.begin(-1, -1, eMMC_MODE, MODE_1_BIT);

    if (initialize_emmc.on_error)
    {
        terminal.printMessage(TerminalMessage(initialize_emmc.debug_message, "MMC", ERROR, micros()));
        return false;
    }

    if (debugging_enabled)
    {
        terminal.printMessage(TerminalMessage("eMMC initialized", "MMC", INFO, micros(),
                                              micros() - initial_time)); // How long did SD card take to init
    }

    return true;
}

void SystemOnModule::initLED()
{
    pixels.begin();
    pixels.clear();
    pixels.show();
}

class MyServerCallbacks : public BLEServerCallbacks
{
    // -- On client connect
    void onConnect(BLEServer *pServer)
    {
        bleServer.onClientConnect();
        terminal.printMessage(TerminalMessage("Client connected", "BLE", INFO, micros()));
    };

    // -- On client disconnect
    void onDisconnect(BLEServer *pServer)
    {
        bleServer.onClientDisconnect();
        terminal.printMessage(TerminalMessage("Client disconnected", "BLE", INFO, micros()));

        delay(200); // Allow some time for the server to stat advertising again
        bleServer.startAdvertising();
        terminal.printMessage(TerminalMessage("Bluetooth server is advertising", "BLE", INFO, micros()));
    }
} ServerCallbacks;

class MyCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string rxValue = pCharacteristic->getValue();

        String incoming_bluetooth_message = "Incoming data (size: ";
        incoming_bluetooth_message += rxValue.length();
        incoming_bluetooth_message += ") -> ";

        // Store incoming message in string
        if (rxValue.length() > 0)
        {
            for (int i = 0; i < rxValue.length(); i++)
            {
                incoming_bluetooth_message += rxValue[i];
                // TODO: Handle your incoming data as you wish here
            }
            terminal.printMessage(TerminalMessage(incoming_bluetooth_message, "BLE", INFO, micros()));
        }
    }

} CharacteristicCallbacks;

bool SystemOnModule::initBLE()
{
    bleServer.begin(BLE_NAME,
                    SERVICE_UUID,
                    CHARACTERISTIC_UUID,
                    &ServerCallbacks,
                    &CharacteristicCallbacks);

    bleServer.startAdvertising();
    bleServer.setMaxMTUsize(185); // -- Try to negotiate Max size MTU (iOS max. MTU is 185 bytes)

    terminal.printMessage(TerminalMessage("Bluetooth server is advertising", "BLE", INFO, micros()));
}