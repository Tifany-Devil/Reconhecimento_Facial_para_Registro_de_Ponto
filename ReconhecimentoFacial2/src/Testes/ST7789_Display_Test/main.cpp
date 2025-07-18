#include "ST7789.h"
#include <bcm2835.h>
#include <iostream>

#define PIN_DC  RPI_V2_GPIO_P1_22  // GPIO24
#define PIN_RST RPI_V2_GPIO_P1_18  // GPIO25

int main() {
    if (!bcm2835_init() || !bcm2835_spi_begin()) {
        std::cerr << "Erro ao iniciar bcm2835 ou SPI" << std::endl;
        return 1;
    }

    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64); // ~4MHz
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

    ST7789 display(PIN_DC, PIN_RST);
    std::cout << "Iniciando display..." << std::endl;
    display.init();

    std::cout << "Preenchendo tela..." << std::endl;
    display.fillScreen(0xFFFF); // Branco

    bcm2835_delay(5000);

    bcm2835_spi_end();
    bcm2835_close();
    return 0;
}
