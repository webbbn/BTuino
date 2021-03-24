
#include <Arduino.h>
#include <GxTFT.h>
#include <GxIO/STM32DUINO/GxIO_STM32F2_FSMC/GxIO_STM32F2_FSMC.h>
#include <GxCTRL/GxCTRL_ILI9488/GxCTRL_ILI9488.h>
#include <lv_conf.h>
#include <lvgl.h>
#include <malloc.h> // for mallinfo()
#include <unistd.h> // for sbrk()

// Create the TFT interface classes
GxIO_Class io;
GxCTRL_Class controller(io);
GxTFT tft(io, controller, 480, 320);

// Create the LVGL display buffer
static lv_disp_buf_t disp_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10];

#if LV_USE_LOG != 0
// Serial debugging
void log_lvgl(lv_log_level_t level, const char * file, uint32_t line, const char * fname, const char *dsc) {
  Serial.printf("%s: %s(%d) - %s\r\n", file, fname, line, dsc);
  Serial.flush();
}
#endif

// Draw the LVGL graphics to the LCD display.
static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {

  // Tell the TFT interface what area of the screen we are changing
  tft.setWindow(area->x1, area->y1, area->x2, area->y2);

  // Update all the pixels in that region from the LVGL buffer
  for (int y = area->y1; y <= area->y2; y++) {
    for (int x = area->x1; x <= area->x2; x++) {
      tft.pushColor((color_p++)->full, 1);
    }
  }

  // tell lvgl that flushing is done 
  lv_disp_flush_ready(disp);
}

int freeHighMemory()
{
    char top;
#ifdef __arm__
    return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
    return &top - __brkval;
#else  // __arm__
    return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif // __arm__
}

size_t halGetMaxFreeBlock() {
  return freeHighMemory();
}

size_t halGetFreeHeap(void) {
  struct mallinfo chuncks = mallinfo();

  // fordblks
  //    This is the total size of memory occupied by free (not in use) chunks.
  return chuncks.fordblks + freeHighMemory();
}

void setup() {

  // Initialize LVGL
  lv_init();

#if LV_USE_LOG != 0
  // Configure the default UART for debug, etc
  Serial.begin(115200);
  // Register the LVGL Logging function
  lv_log_register_print_cb(log_lvgl);
#endif
  LV_LOG_INFO("LVGL display running on %s - %s", controller.name, io.name);

  // Turn on the backlight to full brightness
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Configure the TFT interface
  tft.init();
  LV_LOG_INFO("TFT initialized.");

  // Initialize LVGL
  lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);
  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 480;
  disp_drv.ver_res = 320;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.buffer = &disp_buf;
  lv_disp_drv_register(&disp_drv);
  LV_LOG_INFO("LVGL initialized.");

  // Create the GUI
  lv_obj_t *label = lv_label_create(lv_scr_act(), NULL);
  lv_label_set_text(label, "Hello from LVGL!");
  lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
  LV_LOG_INFO("GUI creates.");

  // Clear the screen
  tft.fillScreen(BLACK);
}

static int loop_counter = 0;
void loop(void) {
  lv_task_handler();
  delay(5);
  if (loop_counter++ > 200) {
    LV_LOG_INFO("Free blocks: %d  Free heap: %d", halGetMaxFreeBlock(), halGetFreeHeap());
    loop_counter = 0;
  }
}
