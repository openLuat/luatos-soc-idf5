

#ifndef __IDF5_IO_DEF__
#define __IDF5_IO_DEF__

#if CONFIG_IDF_TARGET_ESP32

/* UART */
#define UART1_TX_IO_NUM	(0)
#define UART1_RX_IO_NUM	(1)
#define UART2_TX_IO_NUM	(0)
#define UART2_RX_IO_NUM	(1)
/* I2C */
#define I2C0_SDA_IO_NUM	(4)
#define I2C0_SCL_IO_NUM	(5)
#define I2C1_SDA_IO_NUM	(6)
#define I2C1_SCL_IO_NUM	(7)
/* SPI */
#define SPI2_MISO_IO_NUM	(10)
#define SPI2_MOSI_IO_NUM	(3)
#define SPI2_SCLK_IO_NUM	(2)
#define SPI3_MISO_IO_NUM	(10)
#define SPI3_MOSI_IO_NUM	(3)
#define SPI3_SCLK_IO_NUM	(2)

#elif CONFIG_IDF_TARGET_ESP32C2

/* UART */
#define UART1_TX_IO_NUM	(0)
#define UART1_RX_IO_NUM	(1)
/* I2C */
#define I2C0_SDA_IO_NUM	(4)
#define I2C0_SCL_IO_NUM	(5)
/* SPI */
#define SPI2_MISO_IO_NUM	(10)
#define SPI2_MOSI_IO_NUM	(3)
#define SPI2_SCLK_IO_NUM	(2)

#elif CONFIG_IDF_TARGET_ESP32C3

/* UART */
#define UART1_TX_IO_NUM	(0)
#define UART1_RX_IO_NUM	(1)
/* I2C */
#define I2C0_SDA_IO_NUM	(4)
#define I2C0_SCL_IO_NUM	(5)
/* SPI */
#define SPI2_MISO_IO_NUM	(10)
#define SPI2_MOSI_IO_NUM	(3)
#define SPI2_SCLK_IO_NUM	(2)

#elif CONFIG_IDF_TARGET_ESP32S2

/* UART */
#define UART1_TX_IO_NUM	(0)
#define UART1_RX_IO_NUM	(1)
/* I2C */
#define I2C0_SDA_IO_NUM	(4)
#define I2C0_SCL_IO_NUM	(5)
#define I2C1_SDA_IO_NUM	(6)
#define I2C1_SCL_IO_NUM	(7)
/* SPI */
#define SPI2_MISO_IO_NUM	(16)
#define SPI2_MOSI_IO_NUM	(17)
#define SPI2_SCLK_IO_NUM	(18)
#define SPI3_MISO_IO_NUM	(10)
#define SPI3_MOSI_IO_NUM	(3)
#define SPI3_SCLK_IO_NUM	(2)

#elif CONFIG_IDF_TARGET_ESP32S3

/* UART */
#define UART1_TX_IO_NUM	(1)
#define UART1_RX_IO_NUM	(2)
#define UART2_TX_IO_NUM	(6)
#define UART2_RX_IO_NUM	(7)
/* I2C */
#define I2C0_SDA_IO_NUM	(11)
#define I2C0_SCL_IO_NUM	(12)
#define I2C1_SDA_IO_NUM	(6)
#define I2C1_SCL_IO_NUM	(7)
/* SPI */
#define SPI2_MISO_IO_NUM	(16)
#define SPI2_MOSI_IO_NUM	(17)
#define SPI2_SCLK_IO_NUM	(18)
#define SPI3_MISO_IO_NUM	(10)
#define SPI3_MOSI_IO_NUM	(3)
#define SPI3_SCLK_IO_NUM	(2)

#elif CONFIG_IDF_TARGET_ESP32H2

/* UART */
#define UART1_TX_IO_NUM	(0)
#define UART1_RX_IO_NUM	(1)
/* I2C */
#define I2C0_SDA_IO_NUM	(4)
#define I2C0_SCL_IO_NUM	(5)
/* SPI */
#define SPI2_MISO_IO_NUM	(10)
#define SPI2_MOSI_IO_NUM	(3)
#define SPI2_SCLK_IO_NUM	(2)

#endif



#endif /* __IDF5_IO_DEF__ */
