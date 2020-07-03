#include <osZ.h>
#include <driver.h>

public int drv_init()
{
    return 0;
}

public void drv_irq(uint16_t irq __attribute__((unused)), uint64_t __attribute__((unused)) ticks)
{
}

public void drv_open(dev_t device __attribute__((unused)), uint64_t mode __attribute__((unused)))
{
}

public void drv_close(dev_t device __attribute__((unused)))
{
}

public void drv_read(dev_t device __attribute__((unused)))
{
}

public void drv_write(dev_t device __attribute__((unused)))
{
}

public void drv_ioctl(dev_t device __attribute__((unused)))
{
}

