#include <linux/interrupt.h>
#include <linux/gpio.h>

static int spidev_irq_index = 21;

struct spidev_data {
  //...
  struct semaphore sem;
  int     irq;
};

static irqreturn_t spidev_irq(int irq, void *dev_id)
{
    struct spidev_data	*spidev = dev_id;
    printk( KERN_EMERG "==========spislave ready Interrupt============\n");

    disable_irq_nosync(spidev->irq);
    up(&spidev->sem);

    return IRQ_HANDLED;
}

static int spidev_probe(struct spi_device *spi)
{
  //...
  int rc = 0;
  spidev_irq_index = 21;
  if (gpio_is_valid(spidev_irq_index)) {
		rc = gpio_request(spidev_irq_index, "ts_int");
		if (rc) {
			pr_err("failed request gpio for ts_int\n");
			return -1;
		}
	}
  sem_init(&spidev->sem);
  //gpio_direction_input(spidev_irq_index);
  spidev->irq = gpio_to_irq(spidev_irq_index);
  //INIT_WORK(&spidev->work, gsl_ts_xy_worker);
  rc = request_irq(spidev->irq, spidev_irq, IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING, "test", spidev);
  //...
}

//spidev_write()　/ spidev_read()两个函数中，增加信号量控制和enable_irq()
