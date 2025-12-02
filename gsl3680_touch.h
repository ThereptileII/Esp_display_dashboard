#ifndef _GT911_TOUCH_H
#define _GT911_TOUCH_H
#include <stdio.h>

class gsl3680_touch
{
public:
    gsl3680_touch(int8_t sda_pin, int8_t scl_pin, int8_t rst_pin = -1, int8_t int_pin = -1);

    /**
     * Initialise the touch controller. Returns true on success.
     */
    bool begin();

    /**
     * Read the latest touch coordinates.
     * Optionally returns the touch count and first point strength when provided.
     */
    bool getTouch(uint16_t *x, uint16_t *y, uint8_t *count_out = nullptr, uint16_t *strength_out = nullptr);
    void set_rotation(uint8_t r);

private:
    int8_t _sda, _scl, _rst, _int;
};

#endif
