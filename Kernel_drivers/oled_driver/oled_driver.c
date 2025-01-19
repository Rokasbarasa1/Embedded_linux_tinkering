#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/types.h>


void oled_canvas_clear(void);
void oled_canvas_write(const char *string, u8 string_length, bool flip_letters);
int oled_canvas_show(void);


// ----------------------------------------------------------------------------------------Linux stuff
static dev_t oled_driver_device_number;
static struct class *oled_class;
static struct cdev oled_device;

// Buffer for data
const u16 buffer_size = 250;
static char buffer[250];
static int buffer_pointer;

#define DRIVER_NAME "oled_driver"
#define DRIVER_CLASS "OledClass"

static struct i2c_adapter *oled_i2c_adapter = NULL;
static struct i2c_client *oled_i2c_client = NULL;

#define I2C_BUS_AVAILABLE 1
#define SLAVE_DEVICE_NAME "SSD1306"
#define OLED_I2C_ADDRESS 0x3C

static const struct i2c_device_id oled_id[] = {
    {SLAVE_DEVICE_NAME, 0},
    { }
};

static struct i2c_driver oled_driver = {
    .driver = {
        .name = SLAVE_DEVICE_NAME,
        .owner = THIS_MODULE
    }
};

static struct i2c_board_info oled_i2c_board_info = {
    I2C_BOARD_INFO(SLAVE_DEVICE_NAME, OLED_I2C_ADDRESS)
};



// callback read file
static ssize_t driver_read(struct file *File, char *user_buffer, size_t count, loff_t *offs) {
    int to_copy, not_copied, delta;

	/* Get amount of data to copy */
	to_copy = min_t(size_t, count, buffer_pointer);

    // Copy to user
    not_copied = copy_to_user(user_buffer, buffer, to_copy);

    // Calculate data
    delta = to_copy - not_copied;

    return delta;
}

// callback write file
static ssize_t driver_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs) {
    int to_copy, not_copied, delta;

    // Get amount of data to copy
    oled_canvas_clear();
    to_copy = min_t(size_t, count, sizeof(buffer));

    // Copy from user
    not_copied = copy_from_user(buffer, user_buffer, to_copy);
    buffer_pointer = to_copy;
    oled_canvas_write(user_buffer, count, 0);
    // Calculate data
    delta = to_copy - not_copied;
    oled_canvas_show();


    return delta;
}

// ---------------------------------------------------------------------------------------- OLED driver


// oled_display.h
#define SET_ADDRESSING_MODE 0x20
enum t_memory_addressing_mode {
    MODE_PAGE_ADDRESSING = 0x10,
    MODE_HORIZONTAL_ADDRESSING = 0x00,
    MODE_VERTICAL_ADDRESSING = 0x01,
};

#define SET_COLUMN_ADDRESS 0x21
#define SET_PAGE_ADDRESS 0x22

// SET DISPLAY START LINE 0x40 - 0x7F

#define SET_CONTRAST_CONTROL_BANK0 0x81
#define SET_SEGMENT_REMAP 0xA0
#define SET_SEGMENT_REMAP 0xA1
#define SET_ENTIRE_DISPLAY_ON 0xA5
#define SET_ENTIRE_DISPLAY_NORMAL 0xA4
#define SET_DISPLAY_NORMAL 0xA6
#define SET_DISPLAY_INVERSE 0xA7
#define SET_MULTIPLEXER_RATIO 0xA8
#define SET_DISPLAY_ON 0xAF
#define SET_DISPLAY_OFF 0xAE

// SET START PAGE ADDRESS FOR PAGE ADDRESSING 0xB0 - 0xB7

#define SET_COM_OUTPUT_SCAN_DIRECTION 0xC0
#define SET_COM_OUTPUT_SCAN_DIRECTION 0xC8





#define COMMAND_REG 0x80
#define DATA_REG 0x40
#define TEST_ID_REG 0xD0

#define NORMAL_DISPLAY_CMD 0xAF
#define PAGE_ADDRESSING_MODE 0x02

// volatile i2c_inst_t *m_i2c_port = NULL;

volatile u8 canvas[8][128] = {0};
volatile u8 canvas_row_index = 0;
volatile u8 canvas_column_index = 0;

#define LETTER_HEIGHT 8
#define LETTER_WIDTH 5
#define LETTER_GAP 1

// 32+32+31 = 95
bool ascii_characters[95][8][5] = {
    { // space
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
    },
    { // !
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,0,0,0},
        {0,0,1,0,0},
    },
    { // "
        {0,1,0,1,0},
        {0,1,0,1,0},
        {0,1,0,1,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
    },
    { // #
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,1,0,1,0},
        {1,1,1,1,1},
        {0,1,0,1,0},
        {1,1,1,1,1},
        {0,1,0,1,0},
        {0,0,0,0,0},
    },
    { // $
        {0,0,1,0,0},
        {0,1,1,1,0},
        {1,0,1,0,1},
        {0,1,1,0,0},
        {0,0,1,1,0},
        {1,0,1,0,1},
        {0,1,1,1,0},
        {0,0,1,0,0},
    },
    { // %
        {1,1,0,0,1},
        {1,1,0,1,0},
        {0,0,0,1,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,1,0,0,0},
        {0,1,0,1,1},
        {1,0,0,1,1},
    },
    { // &
        {0,1,1,0,0},
        {1,0,0,1,0},
        {1,0,0,1,0},
        {0,1,1,0,0},
        {1,0,1,0,1},
        {1,0,0,1,0},
        {1,0,0,1,1},
        {0,1,1,0,1},
    },
    { // '
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
    },
    { /* ( */
        {0,0,1,1,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
        {0,0,1,1,0},
    },
    { /* ) */
        {0,1,1,0,0},
        {0,0,0,1,0},
        {0,0,0,1,0},
        {0,0,0,1,0},
        {0,0,0,1,0},
        {0,0,0,1,0},
        {0,0,0,1,0},
        {0,1,1,0,0},
    },
    { // *
        {0,0,0,0,0},
        {0,0,0,0,0},
        {1,0,1,0,1},
        {0,1,1,1,0},
        {1,1,1,1,1},
        {0,1,1,1,0},
        {1,0,1,0,1},
        {0,0,0,0,0},
    },
    { // +
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {1,1,1,1,1},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,0,0,0},
    },
    { // ,
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,1,0},
        {0,0,0,1,0},
        {0,0,1,0,0},
    },
    { // -
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {1,1,1,1,1},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
    },
    { // .
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,1,0,0},
    },
    { // /
        {0,0,0,0,1},
        {0,0,0,0,1},
        {0,0,0,1,0},
        {0,0,0,1,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
    },
    { // 0
        {0,0,0,0,0},
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,1,1},
        {1,0,1,0,1},
        {1,1,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    { // 1
        {0,0,0,0,0},
        {0,0,1,0,0},
        {0,1,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {1,1,1,1,1},
    },
    { // 2
        {0,0,0,0,0},
        {0,1,1,1,0},
        {1,0,0,0,1},
        {0,0,0,0,1},
        {0,0,1,1,0},
        {0,1,0,0,0},
        {1,0,0,0,0},
        {1,1,1,1,1},
    },
    { // 3
        {0,0,0,0,0},
        {0,1,1,1,0},
        {1,0,0,0,1},
        {0,0,0,0,1},
        {0,0,1,1,0},
        {0,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    { // 4
        {0,0,0,0,0},
        {0,0,0,1,1},
        {0,0,1,0,1},
        {0,1,0,0,1},
        {1,0,0,0,1},
        {1,1,1,1,1},
        {0,0,0,0,1},
        {0,0,0,0,1},
    },
    { // 5
        {0,0,0,0,0},
        {1,1,1,1,1},
        {1,0,0,0,0},
        {1,1,1,1,0},
        {0,0,0,0,1},
        {0,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    { // 6
        {0,0,0,0,0},
        {0,0,1,1,0},
        {0,1,0,0,0},
        {1,0,0,0,0},
        {1,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    { // 7
        {0,0,0,0,0},
        {1,1,1,1,1},
        {1,0,0,0,1},
        {0,0,0,0,1},
        {0,0,0,1,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
    },
    { // 8
        {0,0,0,0,0},
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    { // 9
        {0,0,0,0,0},
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,1},
        {0,0,0,0,1},
        {0,0,0,1,0},
        {0,1,1,0,0},
    },
    { // :
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,1,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,1,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
    },
    { // ;
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,1,0,0},
        {0,0,0,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,1,0,0,0},
        {0,0,0,0,0},
    },
    { // <
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,1,1},
        {0,1,1,0,0},
        {1,0,0,0,0},
        {0,1,1,0,0},
        {0,0,0,1,1},
        {0,0,0,0,0},
    },
    { // =
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {1,1,1,1,1},
        {0,0,0,0,0},
        {1,1,1,1,1},
        {0,0,0,0,0},
        {0,0,0,0,0},
    },
    { // >
        {0,0,0,0,0},
        {0,0,0,0,0},
        {1,1,0,0,0},
        {0,0,1,1,0},
        {0,0,0,0,1},
        {0,0,1,1,0},
        {1,1,0,0,0},
        {0,0,0,0,0},
    },
    { // ?
        {0,1,1,1,0},
        {1,0,0,0,1},
        {0,0,0,0,1},
        {0,0,0,1,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,0,0,0},
        {0,0,1,0,0},
    },
    { // @
        {0,1,1,1,0},
        {1,0,0,0,1},
        {0,0,1,0,1},
        {1,0,1,1,1},
        {1,0,1,0,1},
        {1,0,1,1,0},
        {1,0,0,0,0},
        {0,1,1,1,0},
    },
    { // A
        {0,0,0,0,0},
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,1,1,1,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
    },
    { // B
        {0,0,0,0,0},
        {1,1,1,1,0},
        {1,0,0,0,1},
        {1,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,1,1,1,0},
    },
    { // C
        {0,0,0,0,0},
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    { // D
        {0,0,0,0,0},
        {1,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,1,1,1,0},
    },
    { // E
        {0,0,0,0,0},
        {1,1,1,1,1},
        {1,0,0,0,0},
        {1,1,1,0,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,1,1,1,1},
    },
    { // F
        {0,0,0,0,0},
        {1,1,1,1,1},
        {1,0,0,0,0},
        {1,1,1,0,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
    },
    { // G
        {0,0,0,0,0},
        {0,1,1,1,1},
        {1,0,0,0,0},
        {1,0,0,1,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    { // H
        {0,0,0,0,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,1,1,1,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
    },
    { // I
        {0,0,0,0,0},
        {0,1,1,1,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,1,1,1,0},
    },
    { // J
        {0,0,0,0,0},
        {0,0,0,0,1},
        {0,0,0,0,1},
        {0,0,0,0,1},
        {0,0,0,0,1},
        {0,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    { // K
        {0,0,0,0,0},
        {1,0,0,0,1},
        {1,0,0,1,0},
        {1,1,1,0,0},
        {1,0,0,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
    },
    { // L
        {0,0,0,0,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,1,1,1,1},
    },
    { // M
        {0,0,0,0,0},
        {1,0,0,0,1},
        {1,1,0,1,1},
        {1,0,1,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
    },
    { // N
        {0,0,0,0,0},
        {1,0,0,0,1},
        {1,1,0,0,1},
        {1,0,1,0,1},
        {1,0,0,1,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
    },
    { // O
        {0,0,0,0,0},
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    { // P
        {0,0,0,0,0},
        {1,1,1,1,0},
        {1,0,0,0,1},
        {1,1,1,1,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
    },
    { // Q
        {0,0,0,0,0},
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,1,0},
        {0,1,1,0,1},
    },
    { // R
        {0,0,0,0,0},
        {1,1,1,1,0},
        {1,0,0,0,1},
        {1,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
    },
    { // S
        {0,0,0,0,0},
        {0,1,1,1,1},
        {1,0,0,0,0},
        {0,1,1,1,0},
        {0,0,0,0,1},
        {0,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    { // T
        {0,0,0,0,0},
        {1,1,1,1,1},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
    },
    { // U
        {0,0,0,0,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    { // V
        {0,0,0,0,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,0,1,0},
        {0,1,0,1,0},
        {0,0,1,0,0},
    },
    { // W
        {0,0,0,0,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,1,0,1},
        {1,1,0,1,1},
        {1,0,0,0,1},
    },
    { // X
        {0,0,0,0,0},
        {1,0,0,0,1},
        {0,1,0,1,0},
        {0,0,1,0,0},
        {0,1,0,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
    },
    { // Y
        {0,0,0,0,0},
        {1,0,0,0,1},
        {0,1,0,1,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
    },
    { // Z
        {0,0,0,0,0},
        {1,1,1,1,1},
        {0,0,0,0,1},
        {0,0,0,1,0},
        {0,0,1,0,0},
        {0,1,0,0,0},
        {1,0,0,0,0},
        {1,1,1,1,1},
    },
    { // [
        {0,1,1,1,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
        {0,1,1,1,0},
    },
    { /* \ */
        {1,0,0,0,0},
        {1,0,0,0,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,0,1,0},
        {0,0,0,1,0},
    },
    { /* [ */
        {0,1,1,1,0},
        {0,0,0,1,0},
        {0,0,0,1,0},
        {0,0,0,1,0},
        {0,0,0,1,0},
        {0,0,0,1,0},
        {0,0,0,1,0},
        {0,1,1,1,0},
    },
    { // ^
        {0,0,1,0,0},
        {0,1,0,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
    },
    { // _
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {1,1,1,1,1},
    },
    { // `
        {0,1,0,0,0},
        {0,0,1,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
    },
    { // a
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,1,1,1,0},
        {0,0,0,0,1},
        {0,1,1,1,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    { // b
        {0,0,0,0,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,0,1,1,0},
        {1,1,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,1,1,1,0},
    },
    { // c
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,0},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    { // d
        {0,0,0,0,0},
        {0,0,0,0,1},
        {0,0,0,0,1},
        {0,1,1,0,1},
        {1,0,0,1,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,1},
    },
    { // e
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,1,1,1,1},
        {1,0,0,0,0},
        {0,1,1,1,1},
    },
    { // f
        {0,0,0,0,0},
        {0,0,1,1,0},
        {0,1,0,0,0},
        {1,1,1,1,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
    },
    { // g
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,1,1,1,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,1},
        {0,0,0,0,1},
        {1,1,1,1,0},
    },
    { // h
        {0,0,0,0,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,0,1,1,0},
        {1,1,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
    },
    { // i
        {0,0,0,0,0},
        {0,0,1,0,0},
        {0,0,0,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
    },
    { // j
        {0,0,0,0,0},
        {0,0,0,0,1},
        {0,0,0,0,0},
        {0,0,0,0,1},
        {0,0,0,0,1},
        {0,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    { // k
        {0,0,0,0,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,0,0,1,0},
        {1,0,1,0,0},
        {1,1,0,0,0},
        {1,0,1,0,0},
        {1,0,0,1,0},
    },
    { // l
        {0,0,0,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,0,1,0},
    },
    { // m
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {1,1,0,1,0},
        {1,0,1,0,1},
        {1,0,1,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
    },
    { // n
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {1,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
    },
    { // o
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,1,1,1,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,0},
    },
    { // p
        {0,0,0,0,0},
        {0,0,0,0,0},
        {1,0,1,1,0},
        {1,1,0,0,1},
        {1,0,0,0,1},
        {1,1,1,1,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
    },
    { // q
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,1,1,0,1},
        {1,0,0,1,1},
        {1,0,0,0,1},
        {0,1,1,1,1},
        {0,0,0,0,1},
        {0,0,0,0,1},
    },
    { // r
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {1,0,1,1,0},
        {1,1,0,0,1},
        {1,0,0,0,0},
        {1,0,0,0,0},
        {1,0,0,0,0},
    },
    { // s
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,1,1,1,1},
        {1,0,0,0,0},
        {0,1,1,1,0},
        {0,0,0,0,1},
        {1,1,1,1,0},
    },
    { // t
        {0,0,0,0,0},
        {0,0,1,0,0},
        {0,1,1,1,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,0,1,0},
    },
    { // u
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,1},
    },
    { // v
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,0,1,0},
        {0,0,1,0,0},
    },
    { // w
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,1,0,1},
        {1,0,1,0,1},
        {0,1,1,1,1},
    },
    { // x
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {1,0,0,0,1},
        {0,1,0,1,0},
        {0,0,1,0,0},
        {0,1,0,1,0},
        {1,0,0,0,1},
    },
    { // y
        {0,0,0,0,0},
        {0,0,0,0,0},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {1,0,0,0,1},
        {0,1,1,1,1},
        {0,0,0,0,1},
        {1,1,1,1,0},
    },
    { // z
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {1,1,1,1,1},
        {0,0,0,1,0},
        {0,0,1,0,0},
        {0,1,0,0,0},
        {1,1,1,1,1},
    },
    { /* { */
        {0,0,1,1,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
        {1,0,0,0,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
        {0,1,0,0,0},
        {0,0,1,1,0},
    },
    { // |
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
        {0,0,1,0,0},
    },
    { /* } */
        {0,1,1,0,0},
        {0,0,0,1,0},
        {0,0,0,1,0},
        {0,0,0,0,1},
        {0,0,0,1,0},
        {0,0,0,1,0},
        {0,0,0,1,0},
        {0,1,1,0,0},
    },
    { // ~
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
        {0,1,0,0,0},
        {1,0,1,0,1},
        {0,0,0,1,0},
        {0,0,0,0,0},
        {0,0,0,0,0},
    },
};



int oled_write_command(u8 command){
    u8 buf[2];
    buf[0] = COMMAND_REG;
    buf[1] = command;

    int transmitted = i2c_master_send(oled_i2c_client, buf, 2);

    if(transmitted > 0) return 0;
    else return -1;
}

int oled_set_cursor(u8 x, u8 y){
    int ret = 0;
    if((ret = oled_write_command(0x00 + (x & 0x0F))) < 0) return ret;
    if((ret = oled_write_command(0x10 + ((x >> 4) & 0x0F))) < 0) return ret;
    return oled_write_command(0xB0 + y);
}

int oled_write_data(u8 data){
    u8 buf[2];
    buf[0] = DATA_REG;
    buf[1] = data;
    int transmitted = i2c_master_send(oled_i2c_client, buf, 2);
    
    if(transmitted > 0) return 0;
    else return -1;
}

int oled_turn_on(void){
    return oled_write_command(SET_DISPLAY_ON);
}

int oled_turn_off(void){
    return oled_write_command(SET_DISPLAY_OFF);
}

int oled_set_mode_normal(void){
    // Inverted or normal mode
    return oled_write_command(SET_DISPLAY_NORMAL);
}

int oled_set_mode_inverted(void){
    // Inverted or normal mode
    return oled_write_command(SET_DISPLAY_INVERSE);
}

int oled_set_page_mode(void){
    int ret = 0;
    if((ret = oled_write_command(SET_ADDRESSING_MODE)) < 0) return ret; // Set addressing mode
    return oled_write_command(MODE_PAGE_ADDRESSING); // set page addressing mode
}

int oled_full_clear(void){
    int ret = 0;
    for(uint page = 0; page < 8; page++){
        if((ret = oled_set_cursor(0, page)) < 0) return ret;
        for(uint column = 0; column < 128; column++){
            if((ret = oled_write_data(0x00)) < 0) return ret;
        }
    }
    return oled_set_cursor(0, 0);
}


void oled_canvas_clear(void){
    for(uint page = 0; page < 8; page++){
        for(uint column = 0; column < 128; column++){
            canvas[page][column] = 0;
        }
    }

    canvas_row_index = 0;
    canvas_column_index = 0;
}

void oled_canvas_write(const char *string, u8 string_length, bool flip_letters){
    for(uint i = 0; i < string_length; i++){

        u8 character_number = string[i] - 32;
        // printf("%c %d %d  row index %d  col index %d \n", string[i], string[i], string[i] - 32, canvas_row_index);

        // Handle new line 
        if(string[i] == '\n'){
            canvas_row_index++;
            canvas_column_index = 0;
            continue;
        }

        // If went beyond the edge go to the next line
        if((canvas_column_index + LETTER_WIDTH + LETTER_GAP) > 127 || string[i] == '\n'){
            canvas_row_index++;
            canvas_column_index = 0;
            if(canvas_row_index > 7){
                // Not enough rows on display to continue
                return;
            }
        }
        
        // Start writing the ascii letter to the canvas
        for(uint column = 0; column < LETTER_WIDTH; column++){
            
            // Get a single column of the correct character. Column 0 is the left most column
            u8 column_value = 0;
            for(uint column_row = 0; column_row < LETTER_HEIGHT; column_row++){
                if(flip_letters){
                    column_value |= (ascii_characters[character_number][column_row][column] << (column_row));
                }else{
                    column_value |= (ascii_characters[character_number][column_row][column] << (7-column_row));
                }
            }
            // use canvas indexes but follow which column to write to by column loop
            canvas[canvas_row_index][canvas_column_index+column] = column_value;
        }

        // Increment the column index so that the new letter can start
        canvas_column_index += LETTER_WIDTH + LETTER_GAP; // 5 For letter width 1 for small gap between letters
    }
}

int oled_canvas_show(void){
    int ret = 0;

    //oled_full_clear();
    if((ret = oled_set_cursor(0, 0)) < 0) return ret;
    
    for(uint page = 0; page < 8; page++){
        if((ret = oled_set_cursor(0, page)) < 0) return ret;

        for(uint column = 0; column < 128; column++){
            if((ret = oled_write_data(canvas[page][column])) < 0) return ret;
        }
    }
    return oled_set_cursor(0, 0);
}

void oled_canvas_invert_row(u8 row_index){
    for(uint page = 0; page < 8; page++){
        if(row_index == page){
            for(uint column = 0; column < 128; column++){
                canvas[page][column] = ~canvas[page][column];
            }
            break;
        }
    }
}








static int driver_open(struct inode *device_file, struct file * instance) {
    printk("oled_driver - open was called!\n");
    return 0;
}

static int driver_close(struct inode *device_file, struct file * instance) {
    printk("oled_driver - close was called!\n");
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = driver_open,
    .release = driver_close,
    .read = driver_read,
    .write = driver_write
};








// Called when it is loaded into the kernel
static int __init ModuleInit(void) {
    printk("oled_driver - initializing!\n");

    int ret = -1;

    // Get a device number value
    if (alloc_chrdev_region(&oled_driver_device_number, 0, 1, DRIVER_NAME) < 0) {
        printk("oled_driver - device number could not be allocated!\n");
        return -1;
    }

    printk("oled_driver - device number major - %d, minor - %d was registered!\n", MAJOR(oled_driver_device_number), MINOR(oled_driver_device_number));

    // Create device class
    oled_class = class_create(DRIVER_CLASS);
    if (IS_ERR(oled_class)) {
        printk("oled_driver - device class cannot be created!\n");
        goto ClassError;
    }

    // Create device file
    if (device_create(oled_class, NULL, oled_driver_device_number, NULL, DRIVER_NAME) == NULL) {
        printk("oled_driver - cannot create device file!\n");
        goto FileError;
    }

    // Initialize the device with the callback
    cdev_init(&oled_device, &fops);

    // Add device to kernel
    if (cdev_add(&oled_device, oled_driver_device_number, 1) == -1) {
        printk("oled_driver - registration of device through kernel failed!\n");
        goto AddError;
    }
    
    // END OF INITIALIZATION

    // OLED STUFF

    oled_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);

    if(oled_i2c_adapter != NULL){
        oled_i2c_client = i2c_new_client_device(oled_i2c_adapter, &oled_i2c_board_info);
        if(oled_i2c_client != NULL) {
            if(i2c_add_driver(&oled_driver) != -1){
                ret = 0;
            }else{
                printk("oled_driver - Can't add i2c driver\n");
                goto I2cError;
            }
        }

        i2c_put_adapter(oled_i2c_adapter);
    } 

    if(ret < 0){

    }

    // READ ID of oled
    // u8 id = i2c_smbus_read_byte_data(oled_i2c_client, TEST_ID_REG);
    // printk("ID: 0x%x\n", id);

    // Initialize the OLDED


    if((ret = oled_turn_on()) < 0) goto I2cCommunicationError;

    if((ret = oled_set_mode_normal()) < 0) goto I2cCommunicationError;
    if((ret = oled_set_page_mode()) < 0) goto I2cCommunicationError;
    if((ret = oled_write_command(0x8d)) < 0) goto I2cCommunicationError;
    if((ret = oled_write_command(0x14)) < 0) goto I2cCommunicationError;
    if((ret = oled_full_clear()) < 0) goto I2cCommunicationError;
    if((ret = oled_set_cursor(0, 0)) < 0) goto I2cCommunicationError;

    
    for(uint page = 0; page < 8; page++){
        if((ret = oled_set_cursor(0, page)) < 0) goto I2cCommunicationError;

        for(uint column = 0; column < 128; column++){
            if((ret = oled_write_data(0b11111111)) < 0) goto I2cCommunicationError;
        }
    }
    if((ret = oled_set_cursor(0, 0)) < 0) goto I2cCommunicationError;

    return ret;

    // // GPIO functionality ----------------------------------------------------

    // // Init GPIO 4 (516)
    // if(gpio_request(516, "rpi-gpio-516")){
    //     printk("oled_driver - cannot allocate GPIO 4 (516)!\n");
    //     goto AddError;
    // }
    // if(gpio_direction_output(516, 0)){
    //     printk("oled_driver - Can not set GPIO 4 (516) to output!\n");
    //     goto Gpio516Error;
    // }

    // // Init GPIO 17 (529)
    // if(gpio_request(529, "rpi-gpio-529")){
    //     printk("oled_driver - Cannot allocate GPIO 17 (529)!\n");
    //     goto AddError;
    // }
    // if(gpio_direction_input(529)){
    //     printk("oled_driver - Can not set GPIO 17 (529) to input!\n");
    //     goto Gpio529Error;
    // }


    // return 0;

// Cascading in reverse order of initialization
I2cCommunicationError:
    printk("");
Gpio529Error:
	gpio_free(529);
Gpio516Error:
	gpio_free(516);
I2cError:
    i2c_unregister_device(oled_i2c_client);
    i2c_del_driver(&oled_driver);
AddError:
	device_destroy(oled_class, oled_driver_device_number);
FileError:
	class_destroy(oled_class);
ClassError:
	unregister_chrdev_region(oled_driver_device_number, 1);
	return -1;
}

// Called when this module is removed from the kernel
static void __exit ModuleExit(void) {

    i2c_unregister_device(oled_i2c_client);
    i2c_del_driver(&oled_driver);

    gpio_set_value(516, 0);
    gpio_free(516);
    gpio_free(529);


    cdev_del(&oled_device);
    device_destroy(oled_class, oled_driver_device_number);
    class_destroy(oled_class);
    unregister_chrdev(oled_driver_device_number, DRIVER_NAME);
    printk("oled_driver - deinitialized!\n");
}

module_init(ModuleInit)
module_exit(ModuleExit)

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rokas Barasa GNU/Linux");
MODULE_DESCRIPTION("A simple driver for setting a led and reading a button");