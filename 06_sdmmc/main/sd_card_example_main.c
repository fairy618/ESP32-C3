// This example uses SDMMC peripheral to communicate with SD card.

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "esp_timer.h"

static const char *TAG = "example";

#define MOUNT_POINT "/sdcard"

#define TEST_FILE_SIZE (4 * 1024 * 1024)

void testReadFile(const char *path, char *buf, int len);
void testWriteFile(const char *path, char *buf, int len);

void app_main(void)
{
    esp_err_t ret;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.

    ESP_LOGI(TAG, "Using SDMMC peripheral");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    // To use 1-line SD mode, change this to 1:
    slot_config.width = 1;

    // On chips where the GPIOs used for SD card can be configured, set them in
    // the slot_config structure:
#ifdef SOC_SDMMC_USE_GPIO_MATRIX
    slot_config.clk = GPIO_NUM_10;
    slot_config.cmd = GPIO_NUM_11;
    slot_config.d0 = GPIO_NUM_9;
    // slot_config.d1 = GPIO_NUM_4;
    // slot_config.d2 = GPIO_NUM_12;
    // slot_config.d3 = GPIO_NUM_13;
#endif

    // Enable internal pullups on enabled pins. The internal pullups
    // are insufficient however, please make sure 10k external pullups are
    // connected on the bus. This is for debug / example purpose only.
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    // Use POSIX and C standard library functions to work with files:

    const char *file_w = MOUNT_POINT "/write.txt";

    char *buf_ = "6666";

    testWriteFile(file_w, buf_, sizeof(buf_));

    testReadFile(file_w, buf_, sizeof(buf_));
}

void testWriteFile(const char *path, char *buf, int len)
{
    unsigned long start_time = esp_timer_get_time() / 1000;
    ESP_LOGI(TAG, "Test write %s", path);

    FILE *file_ = fopen(path, "wb");
    if (!file_)
    {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    int loop = TEST_FILE_SIZE / len;
    while (loop--)
    {
        if (fwrite(buf, 1, len, file_) <= 0)
        {
            ESP_LOGE(TAG, "Write failed");
            return;
        }
    }
    fclose(file_);

    unsigned long time_used = (esp_timer_get_time() / 1000) - start_time;
    printf("Write file used: %ld ms, %f KB/s\n", time_used, (float)TEST_FILE_SIZE / time_used);
}

void testReadFile(const char *path, char *buf, int len)
{
    unsigned long start_time = esp_timer_get_time() / 1000;
    ESP_LOGI(TAG, "Test read %s", path);

    FILE *file_ = fopen(path, "rb");
    if (!file_)
    {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }
    int loop = TEST_FILE_SIZE / len;
    while (loop--)
    {
        if (fread(buf, 1, len, file_) <= 0)
        {
            ESP_LOGE(TAG, "Read failed");
            return;
        }
    }
    fclose(file_);

    unsigned long time_used = esp_timer_get_time() / 1000 - start_time;
    printf("Read file used: %ld ms, %f KB/s\r\n", time_used, (float)TEST_FILE_SIZE / time_used);
}
