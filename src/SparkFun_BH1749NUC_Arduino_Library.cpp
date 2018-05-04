/*
  This is a library written for the BH1749NUC Color Sensor
  SparkFun sells these at its website: www.sparkfun.com
  Do you like this library? Help support SparkFun. Buy a board!
  https://www.sparkfun.com/products/TODO
  Written by Jim Lindblom @ SparkFun Electronics, May 4th, 2018
  The BH1749NUC is a 16-bit RGB and IR color sensor that communicates via an I2C bus.
  The sensor is mounted on our QWIIC RGB Sensor Board, paired with a PCA9536 I/O expander
  which can flip the mounted red, green, blue, and white LED's on or off.
  
  This library handles the initialization of the BH1749NUC and is able to query the sensor
  for different readings.
  https://github.com/sparkfunX/SparkFun_BH1749NUC_Arduino_Library

  Development environment specifics:
  Arduino IDE 1.8.5
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SparkFun_BH1749NUC_Arduino_Library.h>

#ifdef DEBUG_BH1749NUC
#define BH1749NUC_DEBUG(x) if (_debugPort != NULL) { _debugPort->print(x);}
#define BH1749NUC_DEBUGLN(x) if (_debugPort != NULL) { _debugPort->println(x);}
#define STORAGE(x) (x)
#else
#define BH1749NUC_DEBUG(x)
#define BH1749NUC_DEBUGLN(x)
#define STORAGE(x) (x)
#endif

#define BH1749NUC_PART_ID_MASK  0x3F
#define BH1749NUC_PART_ID_SHIFT 0

#define BH1749NUC_IR_GAIN_MASK  0x60
#define BH1749NUC_IR_GAIN_SHIFT 5

#define BH1749NUC_RGB_GAIN_MASK  0x18
#define BH1749NUC_RGB_GAIN_SHIFT 3

#define BH1749NUC_M_MODE_MASK  0x7
#define BH1749NUC_M_MODE_SHIFT 0

#define BH1749NUC_VALID_MASK  0x80
#define BH1749NUC_VALID_SHIFT 7

#define BH1749NUC_ACTIVE_MASK  0x10
#define BH1749NUC_ACTIVE_SHIFT 4

enum {
    BH1749NUC_RED_L,       // 0
    BH1749NUC_RED_H,       // 1
    BH1749NUC_GREEN_L,     // 2
    BH1749NUC_GREEN_H,     // 3
    BH1749NUC_BLUE_L,      // 4
    BH1749NUC_BLUE_H,      // 5
    BH1749NUC_RESERVED_L,  // 6
    BH1749NUC_RESERVED_H,  // 7
    BH1749NUC_IR_L,        // 8
    BH1749NUC_IR_H,        // 9
    BH1749NUC_GREEN2_L,    // 10
    BH1749NUC_GREEN2_H,    // 11
    BH1749NUC_RGB_DATA_LEN // 12
};

enum {
    BH1749NUC_COLOR_L,  // 0
    BH1749NUC_COLOR_H,  // 1
    BH1749NUC_COLOR_LEN // 2
};

#define BH1749NUC_MASK(reg, mask, shift) ((reg & mask) >> shift)

BH1749NUC::BH1749NUC() 
{
    _i2cPort = NULL;
    _deviceAddress = BH1749NUC_ADDRESS_INVALID;
#ifdef DEBUG_BH1749NUC
    _debugPort = NULL;
#endif
}

BH1749NUC_error_t BH1749NUC::begin(BH1749NUC_Address_t deviceAddress, TwoWire &wirePort) 
{
    uint8_t systemControl = 0;
    BH1749NUC_error_t retVal;

    _deviceAddress = deviceAddress;
    _i2cPort = &wirePort;

    _i2cPort->begin();

    retVal = readI2CBuffer(&systemControl, BH1749NUC_REGISTER_SYSTEM_CONTROL, 1);
    if ( retVal != BH1749NUC_ERROR_SUCCESS)
    {
        return retVal;
    }
    if (BH1749NUC_MASK(systemControl, BH1749NUC_PART_ID_MASK, BH1749NUC_PART_ID_SHIFT) 
         != BH1749NUC_PART_ID)
    {
        BH1749NUC_DEBUGLN((STORAGE("ERR (begin): Part ID mismatch")));
        return BH1749NUC_ERROR_PART_ID;
    }

    setMeasurementActive(true);

    return BH1749NUC_ERROR_SUCCESS;
}

boolean BH1749NUC::begin()
{
    if (begin(BH1749NUC_ADDRESS_DEFAULT, Wire) == BH1749NUC_ERROR_SUCCESS)
    {
        return true;
    }
    return false;
}

void BH1749NUC::setDebugStream(Stream & debugPort)
{
    _debugPort = &debugPort;
}


BH1749NUC_gain_t BH1749NUC::readIRGain(void)
{
    uint8_t rawRegister;
    BH1749NUC_error_t err;
    BH1749NUC_gain_t retVal;

    err = readI2CRegister(&rawRegister, BH1749NUC_REGISTER_MODE_CONTROL1);
    if (err  != BH1749NUC_ERROR_SUCCESS)
    {
        return BH1749NUC_GAIN_INVALID;
    }
    retVal = (BH1749NUC_gain_t) BH1749NUC_MASK(rawRegister, BH1749NUC_IR_GAIN_MASK, BH1749NUC_IR_GAIN_SHIFT);
    return retVal;
}

BH1749NUC_error_t BH1749NUC::setIRGain(BH1749NUC_gain_t gain)
{
    uint8_t rawRegister = 0;
    BH1749NUC_error_t err;
    BH1749NUC_gain_t retVal;

    if ((gain != BH1749NUC_GAIN_X1) && (gain != BH1749NUC_GAIN_X32))
    {
        return BH1749NUC_ERROR_UNDEFINED;
    }

    err = readI2CRegister(&rawRegister, BH1749NUC_REGISTER_MODE_CONTROL1);
    if (err  != BH1749NUC_ERROR_SUCCESS)
    {
        return err;
    }

    rawRegister &= ~(BH1749NUC_IR_GAIN_MASK); // Clear mask bits
    rawRegister |= (gain << BH1749NUC_IR_GAIN_SHIFT); // Shift in new bits
    
    err = writeI2CRegister(rawRegister, BH1749NUC_REGISTER_MODE_CONTROL1);
    return err;
}

BH1749NUC_gain_t BH1749NUC::readRGBGain(void)
{
    uint8_t rawRegister;
    BH1749NUC_error_t err;
    BH1749NUC_gain_t retVal;

    err = readI2CRegister(&rawRegister, BH1749NUC_REGISTER_MODE_CONTROL1);
    if (err  != BH1749NUC_ERROR_SUCCESS)
    {
        return BH1749NUC_GAIN_INVALID;
    }
    retVal = (BH1749NUC_gain_t) BH1749NUC_MASK(rawRegister, BH1749NUC_RGB_GAIN_MASK, BH1749NUC_RGB_GAIN_SHIFT);
    return retVal;
}

BH1749NUC_error_t BH1749NUC::setRGBGain(BH1749NUC_gain_t gain)
{
    uint8_t rawRegister = 0;
    BH1749NUC_error_t err;
    BH1749NUC_gain_t retVal;

    if ((gain != BH1749NUC_GAIN_X1) && (gain != BH1749NUC_GAIN_X32))
    {
        return BH1749NUC_ERROR_UNDEFINED;
    }

    err = readI2CRegister(&rawRegister, BH1749NUC_REGISTER_MODE_CONTROL1);
    if (err  != BH1749NUC_ERROR_SUCCESS)
    {
        return err;
    }

    rawRegister &= ~(BH1749NUC_RGB_GAIN_MASK); // Clear mask bits
    rawRegister |= (gain << BH1749NUC_RGB_GAIN_SHIFT); // Shift in new bits
    
    err = writeI2CRegister(rawRegister, BH1749NUC_REGISTER_MODE_CONTROL1);
    return err;
}

BH1749NUC_measurement_mode_t BH1749NUC::readMeasurementMode(void)
{
    uint8_t rawRegister;
    BH1749NUC_error_t err;
    BH1749NUC_measurement_mode_t retVal;

    err = readI2CRegister(&rawRegister, BH1749NUC_REGISTER_MODE_CONTROL1);
    if (err  != BH1749NUC_ERROR_SUCCESS)
    {
        return BH1749NUC_MEASUREMENT_MODE_INVALID;
    }
    retVal = (BH1749NUC_measurement_mode_t) BH1749NUC_MASK(rawRegister, BH1749NUC_M_MODE_MASK, BH1749NUC_M_MODE_SHIFT);
    return retVal;
}

BH1749NUC_error_t BH1749NUC::setMeasurementMode(BH1749NUC_measurement_mode_t mode)
{
    uint8_t rawRegister = 0;
    BH1749NUC_error_t err;
    BH1749NUC_gain_t retVal;

    if ((mode != BH1749NUC_MEASUREMENT_MODE_120_MS) && (mode != BH1749NUC_MEASUREMENT_MODE_240_MS) &&
        (mode != BH1749NUC_MEASUREMENT_MODE_35_MS))
    {
        return BH1749NUC_ERROR_UNDEFINED;
    }

    err = readI2CRegister(&rawRegister, BH1749NUC_REGISTER_MODE_CONTROL1);
    if (err  != BH1749NUC_ERROR_SUCCESS)
    {
        return err;
    }

    rawRegister &= ~(BH1749NUC_M_MODE_MASK); // Clear mask bits
    rawRegister |= (mode << BH1749NUC_M_MODE_SHIFT); // Shift in new bits
    
    err = writeI2CRegister(rawRegister, BH1749NUC_REGISTER_MODE_CONTROL1);
    return err;
}

boolean BH1749NUC::update(void)
{
    if (read(&this->colors) == BH1749NUC_ERROR_SUCCESS)
    {
        return true;
    }
    return false;
}
uint16_t BH1749NUC::read(BH1749NUC_color_t color)
{
    uint16_t retVal;
    switch (color)
    {
    case BH1749NUC_RED:
        retVal = readRed();
        break;
    case BH1749NUC_GREEN:
        retVal = readGreen();
        break;
    case BH1749NUC_BLUE:
        retVal = readBlue();
        break;
    case BH1749NUC_IR:
        retVal = readIR();
        break;
    case BH1749NUC_GREEN2:
        retVal = readGreen2();
        break;
    }

    return retVal;
}

BH1749NUC_error_t BH1749NUC::read(rgb_sense * rgb)
{
    uint8_t rawColorData[BH1749NUC_RGB_DATA_LEN];
    BH1749NUC_error_t err;
    err = readI2CBuffer(rawColorData, BH1749NUC_REGISTER_RED_DATA_L, BH1749NUC_RGB_DATA_LEN);
    if (err != BH1749NUC_ERROR_SUCCESS)
    {
        return err;
    }
    rgb->red = rawColorData[BH1749NUC_RED_L] | (rawColorData[BH1749NUC_RED_H] << 8);
    rgb->green = rawColorData[BH1749NUC_GREEN_L] | (rawColorData[BH1749NUC_GREEN_H] << 8);
    rgb->blue = rawColorData[BH1749NUC_BLUE_L] | (rawColorData[BH1749NUC_BLUE_H] << 8);
    rgb->ir = rawColorData[BH1749NUC_IR_L] | (rawColorData[BH1749NUC_IR_H] << 8);
    rgb->green2 = rawColorData[BH1749NUC_GREEN2_L] | (rawColorData[BH1749NUC_GREEN2_H] << 8);

    return BH1749NUC_ERROR_SUCCESS;
}

uint16_t BH1749NUC::readRed(void)
{
    uint8_t rawRedData[BH1749NUC_COLOR_LEN];
    BH1749NUC_error_t err;
    err = readI2CBuffer(rawRedData, BH1749NUC_REGISTER_RED_DATA_L, BH1749NUC_COLOR_LEN);
    if (err != BH1749NUC_ERROR_SUCCESS)
    {
        return 0;
    }
    return rawRedData[BH1749NUC_COLOR_L] | (rawRedData[BH1749NUC_COLOR_H] << 8);
}

uint16_t BH1749NUC::readGreen(void)
{
    uint8_t rawGreenData[BH1749NUC_COLOR_LEN];
    BH1749NUC_error_t err;
    err = readI2CBuffer(rawGreenData, BH1749NUC_REGISTER_GREEN_DATA_L, BH1749NUC_COLOR_LEN);
    if (err != BH1749NUC_ERROR_SUCCESS)
    {
        return 0;
    }
    return rawGreenData[BH1749NUC_COLOR_L] | (rawGreenData[BH1749NUC_COLOR_H] << 8);
}

uint16_t BH1749NUC::readGreen2(void)
{
    uint8_t rawGreen2Data[BH1749NUC_COLOR_LEN];
    BH1749NUC_error_t err;
    err = readI2CBuffer(rawGreen2Data, BH1749NUC_REGISTER_GREEN2_DATA_L, BH1749NUC_COLOR_LEN);
    if (err != BH1749NUC_ERROR_SUCCESS)
    {
        return 0;
    }
    return rawGreen2Data[BH1749NUC_COLOR_L] | (rawGreen2Data[BH1749NUC_COLOR_H] << 8);
}

uint16_t BH1749NUC::readBlue(void)
{
    uint8_t rawBlueData[BH1749NUC_COLOR_LEN];
    BH1749NUC_error_t err;
    err = readI2CBuffer(rawBlueData, BH1749NUC_REGISTER_BLUE_DATA_L, BH1749NUC_COLOR_LEN);
    if (err != BH1749NUC_ERROR_SUCCESS)
    {
        return 0;
    }
    return rawBlueData[BH1749NUC_COLOR_L] | (rawBlueData[BH1749NUC_COLOR_H] << 8);
}

uint16_t BH1749NUC::readIR(void)
{
    uint8_t rawIRData[BH1749NUC_COLOR_LEN];
    BH1749NUC_error_t err;
    err = readI2CBuffer(rawIRData, BH1749NUC_REGISTER_IR_DATA_L, BH1749NUC_COLOR_LEN);
    if (err != BH1749NUC_ERROR_SUCCESS)
    {
        return 0;
    }
    return rawIRData[BH1749NUC_COLOR_L] | (rawIRData[BH1749NUC_COLOR_H] << 8);
}

boolean BH1749NUC::ready(void)
{
    if (readValid() == BH1749NUC_MEASUREMENT_VALID_VALID)
    {
        return true;
    }
    return false;
}

// Private
BH1749NUC::BH1749NUC_measurement_valid_t BH1749NUC::readValid(void)
{
    uint8_t rawRegister;
    BH1749NUC_error_t err;
    BH1749NUC_measurement_valid_t retVal;

    err = readI2CRegister(&rawRegister, BH1749NUC_REGISTER_MODE_CONTROL2);
    if (err  != BH1749NUC_ERROR_SUCCESS)
    {
        return BH1749NUC_MEASUREMENT_VALID_INVALID_INVALID;
    }
    retVal = (BH1749NUC_measurement_valid_t) BH1749NUC_MASK(rawRegister, BH1749NUC_VALID_MASK, BH1749NUC_VALID_SHIFT);
    return retVal;
}

BH1749NUC::BH1749NUC_measurement_active_t BH1749NUC::readMeasurementActive(void)
{
    uint8_t rawRegister;
    BH1749NUC_error_t err;
    BH1749NUC_measurement_active_t retVal;

    err = readI2CRegister(&rawRegister, BH1749NUC_REGISTER_MODE_CONTROL2);
    if (err  != BH1749NUC_ERROR_SUCCESS)
    {
        return BH1749NUC_MEASUREMENT_ACTIVE_INVALID;
    }
    retVal = (BH1749NUC_measurement_active_t) BH1749NUC_MASK(rawRegister, BH1749NUC_ACTIVE_MASK, BH1749NUC_ACTIVE_SHIFT);
    return retVal;
}

BH1749NUC_error_t BH1749NUC::setMeasurementActive(boolean active)
{
    uint8_t rawRegister = 0;
    BH1749NUC_error_t err;
    BH1749NUC_gain_t retVal;

    err = readI2CRegister(&rawRegister, BH1749NUC_REGISTER_MODE_CONTROL2);
    if (err  != BH1749NUC_ERROR_SUCCESS)
    {
        return err;
    }

    rawRegister &= ~(BH1749NUC_ACTIVE_MASK); // Clear mask bits
    rawRegister |= ((uint8_t)active << BH1749NUC_ACTIVE_SHIFT); // Shift in new bits
    
    err = writeI2CRegister(rawRegister, BH1749NUC_REGISTER_MODE_CONTROL2);
    return err;
}

BH1749NUC_error_t BH1749NUC::readI2CBuffer(uint8_t * dest, BH1749NUC_REGISTER_t startRegister, uint16_t len)
{
    BH1749NUC_DEBUGLN((STORAGE("(readI2CBuffer): read ") + String(len) + 
                       STORAGE(" @ 0x") + String(startRegister, HEX)));
    if (_deviceAddress == BH1749NUC_ADDRESS_INVALID)
    {
        BH1749NUC_DEBUGLN(STORAGE("    ERR (readI2CBuffer): Invalid address"));
        return BH1749NUC_ERROR_INVALID_ADDRESS;
    }
    _i2cPort->beginTransmission((uint8_t) _deviceAddress);
    _i2cPort->write(startRegister);
    if (_i2cPort->endTransmission(false) != 0)
    {
        BH1749NUC_DEBUGLN(STORAGE("    ERR (readI2CBuffer): End transmission"));
        return BH1749NUC_ERROR_READ;
    }
    
    _i2cPort->requestFrom((uint8_t)_deviceAddress, (uint8_t)len);
    for (int i = 0; i < len; i++)
    {
        dest[i] = _i2cPort->read();
        BH1749NUC_DEBUGLN((STORAGE("    ") + String(i) + STORAGE(": 0x") + String(dest[i], HEX)));
    }

    return BH1749NUC_ERROR_SUCCESS;
}

BH1749NUC_error_t BH1749NUC::writeI2CBuffer(uint8_t * src, BH1749NUC_REGISTER_t startRegister, uint16_t len)
{
    if (_deviceAddress == BH1749NUC_ADDRESS_INVALID)
    {
        BH1749NUC_DEBUGLN(STORAGE("ERR (readI2CBuffer): Invalid address"));
        return BH1749NUC_ERROR_INVALID_ADDRESS;
    }
    _i2cPort->beginTransmission((uint8_t) _deviceAddress);
    _i2cPort->write(startRegister);
    for (int i = 0; i < len; i++)
    {
        _i2cPort->write(src[i]);
    }
    if (_i2cPort->endTransmission(true) != 0)
    {
        return BH1749NUC_ERROR_WRITE;
    }
    return BH1749NUC_ERROR_SUCCESS;
}

BH1749NUC_error_t BH1749NUC::readI2CRegister(uint8_t * dest, BH1749NUC_REGISTER_t registerAddress)
{
    return readI2CBuffer(dest, registerAddress, 1);
}

BH1749NUC_error_t BH1749NUC::writeI2CRegister(uint8_t data, BH1749NUC_REGISTER_t registerAddress)
{
    return writeI2CBuffer(&data, registerAddress, 1);
}