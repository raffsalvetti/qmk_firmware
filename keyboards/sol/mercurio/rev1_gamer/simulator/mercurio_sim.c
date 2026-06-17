#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#include "sim_avr.h"
#include "avr_ioport.h"
#include "avr_twi.h"
#include "avr_uart.h"
#include "avr_adc.h"
#include "sim_hex.h"
#include "sim_vcd_file.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <math.h>

// simavr/parts
#include "i2c_eeprom.h"
#include "ssd1306_virt.h"

avr_t * avr = NULL;
avr_vcd_t vcd_file;
i2c_eeprom_t eeprom;
ssd1306_t oled;

// OLED Module Configuration
int oled_mod_pos_x = 0;
int oled_mod_pos_y = 0;
int oled_mod_rend_x = 0;
int oled_mod_rend_y = 0;
SDL_Texture *oled_face_tex = NULL;
int oled_face_w = 0;
int oled_face_h = 0;

// Simulated EEPROM backing store
uint8_t eeprom_data[4096];

// Phase 2: WS2812 State
uint32_t ws2812_colors[35]; // 35 RGB LEDs
static uint64_t ws2812_high_start = 0;
static uint64_t ws2812_low_start = 0;
static int ws2812_bit_idx = 0;

void ws2812_pin_notify(struct avr_irq_t * irq, uint32_t value, void * param) {
    avr_t * avr = (avr_t*)param;
    if (value) {
        // Pin went HIGH
        uint64_t reset_delta = avr->cycle - ws2812_low_start;
        if (reset_delta > 800) {
            // Reset pulse (> 50us)
            // (Optional: trigger an SDL redraw here since frame is complete)
            ws2812_bit_idx = 0;
        }
        ws2812_high_start = avr->cycle;
    } else {
        // Pin went LOW
        uint64_t pulse_width = avr->cycle - ws2812_high_start;
        ws2812_low_start = avr->cycle;
        
        if (ws2812_bit_idx < 35 * 24) {
            uint32_t pixel_idx = ws2812_bit_idx / 24;
            uint8_t bit = (pulse_width >= 10) ? 1 : 0; // >= 0.6us is a '1'
            if (ws2812_bit_idx % 24 == 0) ws2812_colors[pixel_idx] = 0;
            if (bit) {
                ws2812_colors[pixel_idx] |= (1 << (23 - (ws2812_bit_idx % 24)));
            }
            ws2812_bit_idx++;
        }
    }
}

// Phase 4: Audio State
volatile double current_audio_freq = 0.0;
static uint64_t last_piezo_toggle = 0;

void piezo_pin_notify(struct avr_irq_t * irq, uint32_t value, void * param) {
    avr_t * avr = (avr_t*)param;
    uint64_t now = avr->cycle;
    uint64_t delta = now - last_piezo_toggle;
    last_piezo_toggle = now;
    
    // Ignore very slow toggles (less than ~100Hz)
    if (delta > 0 && delta < 80000) { 
        double freq = 16000000.0 / (2.0 * (double)delta);
        if (freq != current_audio_freq) {
            current_audio_freq = freq;
            // printf("SIM: Piezo Frequency = %.1f Hz\n", freq);
        }
    }
}

// Phase 4: SDL Audio Callback
void sdl_audio_callback(void *userdata, Uint8 *stream, int len) {
    Sint16 *buffer = (Sint16 *)stream;
    int samples = len / 2;
    static double phase = 0.0;
    
    double freq = current_audio_freq; // Capture volatile
    
    for (int i = 0; i < samples; i++) {
        if (freq > 20.0) {
            double phase_inc = (freq / 44100.0) * 2.0 * M_PI;
            phase += phase_inc;
            if (phase >= 2.0 * M_PI) phase -= 2.0 * M_PI;
            // Square wave generator (8000 amplitude)
            buffer[i] = (sin(phase) > 0) ? 8000 : -8000;
        } else {
            buffer[i] = 0;
            phase = 0.0;
        }
    }
}

// Phase 2: LDR Injection
void inject_ldr_voltage(avr_t * avr, uint16_t millivolts) {
    avr_irq_t * adc_irq = avr_io_getirq(avr, AVR_IOCTL_ADC_GETIRQ, 6); // ADC6 (PA6)
    if (adc_irq) avr_raise_irq(adc_irq, millivolts);
}

// PS/2 Mouse Injection Helper
void inject_ps2_byte(avr_t * avr, uint8_t byte) {
    avr_irq_t * uart_in = avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_INPUT);
    if (uart_in) avr_raise_irq(uart_in, byte);
}

// PS/2 Mouse Injection Callback
static avr_cycle_count_t inject_mouse_byte(struct avr_t *avr, avr_cycle_count_t when, void *param) {
    uint8_t byte = (uint8_t)(intptr_t)param;
    inject_ps2_byte(avr, byte);
    printf("SIM: Injected PS/2 Byte: 0x%02X\n", byte);
    return 0; // Do not reschedule
}

// Fake USB Enumeration Injection
static avr_cycle_count_t inject_usb_config(struct avr_t *avr, avr_cycle_count_t when, void *param) {
#ifdef USB_CONFIG_ADDR
    uint16_t sram_addr = (USB_CONFIG_ADDR) & 0xFFFF;
    avr->data[sram_addr] = 1;
    printf("SIM: Faked USB Enumeration (usbConfiguration=1 at 0x%04X)\n", sram_addr);
#endif
#ifdef USB_TXLEN_ADDR
    uint16_t txlen_addr = (USB_TXLEN_ADDR) & 0xFFFF;
    avr->data[txlen_addr] = 0xFF; // -1
    printf("SIM: Faked USB Ready (usbTxLen=-1 at 0x%04X)\n", txlen_addr);
#endif
    return 0; // Do not reschedule
}

static avr_cycle_count_t inject_usb_sof(struct avr_t *avr, avr_cycle_count_t when, void *param) {
#ifdef USB_SOF_ADDR
    uint16_t sof_addr = (USB_SOF_ADDR) & 0xFFFF;
    avr->data[sof_addr] = 1;
#endif
    return when + 16000; // Reschedule every 1ms
}

// RGB LED Configuration
typedef struct {
    int id;
    int x;
    int y;
    int radius;
} led_position_t;

led_position_t led_positions[35];

// Key Position Configuration
typedef struct {
    int row;
    int col;
    int x;
    int y;
    int size;
    int angle;
} key_position_t;

key_position_t key_positions[35];
int key_position_count = 0;

// SDL Fill Circle Helper (No overlapping lines for correct alpha blending)
void SDL_RenderFillCircle(SDL_Renderer * renderer, int center_x, int center_y, int radius) {
    for (int y = -radius; y <= radius; y++) {
        int x = (int)sqrt(radius * radius - y * y);
        SDL_RenderDrawLine(renderer, center_x - x, center_y + y, center_x + x, center_y + y);
    }
}

int main(int argc, char *argv[]) {
    // 1. Initialize AVR Core
    const char * hex_path = "../../../../../.build/sol_mercurio_rev1_gamer_default.hex";
    uint32_t boot_base, boot_size;
    uint8_t * boot = read_ihex_file(hex_path, &boot_size, &boot_base);

    if (!boot) {
        fprintf(stderr, "Error loading HEX firmware: %s\n", hex_path);
        return 1;
    }

    printf("Firmware loaded: %s (%d bytes)\n", hex_path, boot_size);

    avr = avr_make_mcu_by_name("atmega32");
    if (!avr) {
        fprintf(stderr, "Error creating AVR core\n");
        return 1;
    }

    avr_init(avr);
    avr->frequency = 16000000;
    memcpy(avr->flash + boot_base, boot, boot_size);
    free(boot);
    avr->pc = boot_base;
    avr->codeend = avr->flashend;

    // 2. Initialize I2C EEPROM (24LC32 or similar, address 0xA0)
    // Try to load existing eeprom.bin
    int fd = open("eeprom.bin", O_RDONLY);
    if (fd >= 0) {
        read(fd, eeprom_data, sizeof(eeprom_data));
        close(fd);
    } else {
        memset(eeprom_data, 0xFF, sizeof(eeprom_data));
    }

    i2c_eeprom_init(avr, &eeprom, 0xA0, 0x01, eeprom_data, sizeof(eeprom_data));
    i2c_eeprom_attach(avr, &eeprom, AVR_IOCTL_TWI_GETIRQ(0));

    // 3. Initialize I2C OLED SSD1306 (address 0x78 = 0x3C << 1)
    ssd1306_init(avr, &oled, 128, 64);
    // Wire the OLED reset pin to an unused dummy pin (e.g., PORTD pin 7) to avoid overlapping with keyboard matrix (A0)
    ssd1306_wiring_t oled_wiring = { .reset = { .port = 'D', .pin = 7 } };
    ssd1306_connect_twi(&oled, &oled_wiring);

    // 4. Setup VCD Tracing
    avr_vcd_init(avr, "trace.vcd", &vcd_file, 100);
    // Trace USART RX/TX (PD0, PD1)
    avr_irq_t * rxd_irq = avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), IOPORT_IRQ_PIN0);
    avr_irq_t * txd_irq = avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), IOPORT_IRQ_PIN1);
    if (rxd_irq) avr_vcd_add_signal(&vcd_file, rxd_irq, 1, "RXD");
    if (txd_irq) avr_vcd_add_signal(&vcd_file, txd_irq, 1, "TXD");

    // Trace V-USB D- (PD3) and D+ (PD4)
    avr_irq_t * dminus_irq = avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), IOPORT_IRQ_PIN3);
    avr_irq_t * dplus_irq = avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), IOPORT_IRQ_PIN4);
    if (dminus_irq) avr_vcd_add_signal(&vcd_file, dminus_irq, 1, "USB_D-");
    if (dplus_irq) avr_vcd_add_signal(&vcd_file, dplus_irq, 1, "USB_D+");
    
    avr_vcd_start(&vcd_file);

    // Phase 2: Hook WS2812 Pin (PB4 = IOPORT B, PIN 4)
    avr_irq_t * ws2812_irq = avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), 4);
    if (ws2812_irq) {
        avr_irq_register_notify(ws2812_irq, ws2812_pin_notify, avr);
        printf("SIM: Registered WS2812 decoder on PB4\n");
    }

    // Phase 4: Hook Piezo Pin (PD7 = IOPORT D, PIN 7)
    avr_irq_t * piezo_irq = avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), 7);
    if (piezo_irq) {
        avr_irq_register_notify(piezo_irq, piezo_pin_notify, avr);
        printf("SIM: Registered Piezo decoder on PD7\n");
    }

    // Phase 2: Inject initial LDR voltage (5.0V for maximum LED brightness)
    inject_ldr_voltage(avr, 5000);

    // Fake USB Enumeration 0.5 seconds after boot (16Mhz * 0.5 = 8M cycles)
    avr_cycle_timer_register(avr, 8000000, inject_usb_config, NULL);

    // Fake USB SOF (Start of Frame) every 1ms to prevent QMK from suspending the CPU
    avr_cycle_timer_register(avr, 16000, inject_usb_sof, NULL);



    // 5. Schedule PS/2 Mouse Injection
    // The firmware waits 8.5 seconds before initializing the mouse.
    // Let's inject a valid PS/2 packet at 9.0 seconds.
    uint64_t cycles_9s = avr->frequency * 9;
    
    // Packet: Left Click (0x09), X=+10 (0x0A), Y=-5 (0xFB)
    avr_cycle_timer_register(avr, cycles_9s, inject_mouse_byte, (void*)(intptr_t)0x09);
    avr_cycle_timer_register(avr, cycles_9s + 5000, inject_mouse_byte, (void*)(intptr_t)0x0A);
    avr_cycle_timer_register(avr, cycles_9s + 10000, inject_mouse_byte, (void*)(intptr_t)0xFB);

    printf("Starting Simulation. Press Ctrl+C to stop.\n");

    // 6. Start simavr in a separate thread so it doesn't block the GUI
    pthread_t avr_thread;
    int sim_running = 1;
    
    void* avr_run_thread(void* arg) {
        int state = cpu_Running;
        uint64_t last_sync = 0;
        while (sim_running && state != cpu_Done && state != cpu_Crashed) {
            state = avr_run(avr);
            // Throttle to roughly 16MHz (16,000,000 cycles per sec)
            // Sync every 160,000 cycles (10ms)
            if (avr->cycle - last_sync >= 160000) {
                usleep(10000); 
                last_sync = avr->cycle;
            }
        }
        return NULL;
    }
    // We will start the thread AFTER SDL initialization so we don't miss the first frames

    // Load LED positions from CSV
    for (int i = 0; i < 35; i++) {
        led_positions[i].id = i;
        led_positions[i].x = 100 + (i % 7) * 80;
        led_positions[i].y = 100 + (i / 7) * 80;
        led_positions[i].radius = 30; // fallback default
    }
    FILE *led_file = fopen("led_positions.csv", "r");
    if (led_file) {
        char line[256];
        while (fgets(line, sizeof(line), led_file)) {
            if (line[0] == '#' || line[0] == '\n') continue;
            int id, x, y, r;
            if (sscanf(line, "%d, %d, %d, %d", &id, &x, &y, &r) == 4) {
                if (id >= 0 && id < 35) {
                    led_positions[id].x = x;
                    led_positions[id].y = y;
                    led_positions[id].radius = r;
                }
            }
        }
        fclose(led_file);
        printf("SIM: Loaded led_positions.csv\n");
    } else {
        printf("SIM: Warning: led_positions.csv not found, using defaults\n");
    }

    // Load key positions from CSV
    FILE *key_file = fopen("key_positions.csv", "r");
    if (key_file) {
        char line[256];
        while (fgets(line, sizeof(line), key_file)) {
            if (line[0] == '#' || line[0] == '\n') continue;
            int r, c, x, y, size, angle;
            if (sscanf(line, "%d, %d, %d, %d, %d, %d", &r, &c, &x, &y, &size, &angle) == 6) {
                if (key_position_count < 35) {
                    key_positions[key_position_count].row = r;
                    key_positions[key_position_count].col = c;
                    key_positions[key_position_count].x = x;
                    key_positions[key_position_count].y = y;
                    key_positions[key_position_count].size = size;
                    key_positions[key_position_count].angle = angle;
                    key_position_count++;
                }
            }
        }
        fclose(key_file);
        printf("SIM: Loaded key_positions.csv (%d keys)\n", key_position_count);
    } else {
        printf("SIM: Warning: key_positions.csv not found\n");
    }

    // Load OLED Module Config
    FILE *oled_ini = fopen("oled_module.ini", "r");
    if (oled_ini) {
        char line[256];
        while (fgets(line, sizeof(line), oled_ini)) {
            if (strncmp(line, "position-x =", 12) == 0) oled_mod_pos_x = atoi(line + 12);
            if (strncmp(line, "position-y =", 12) == 0) oled_mod_pos_y = atoi(line + 12);
            if (strncmp(line, "render-x =", 10) == 0) oled_mod_rend_x = atoi(line + 10);
            if (strncmp(line, "render-y =", 10) == 0) oled_mod_rend_y = atoi(line + 10);
        }
        fclose(oled_ini);
        printf("SIM: Loaded oled_module.ini (pos: %d,%d, rend: %d,%d)\n", 
            oled_mod_pos_x, oled_mod_pos_y, oled_mod_rend_x, oled_mod_rend_y);
    } else {
        printf("SIM: Warning: oled_module.ini not found\n");
    }

    // 7. Initialize SDL2 GUI & Audio
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        sim_running = 0;
    } else {
        // Setup Audio
        SDL_AudioSpec want, have;
        SDL_zero(want);
        want.freq = 44100;
        want.format = AUDIO_S16SYS;
        want.channels = 1;
        want.samples = 1024;
        want.callback = sdl_audio_callback;
        
        SDL_AudioDeviceID audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        if (audio_dev > 0) {
            SDL_PauseAudioDevice(audio_dev, 0); // Start playing
        } else {
            fprintf(stderr, "Failed to open SDL Audio: %s\n", SDL_GetError());
        }

        // Load background surface to calculate proportions
        SDL_Surface *bg_surf = IMG_Load("mercurio.png");
        int win_width = 1200;
        int win_height = 600; // default if not found
        if (bg_surf) {
            win_height = (win_width * bg_surf->h) / bg_surf->w;
        }

        SDL_Window *win = SDL_CreateWindow("Mercurio Simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, win_width, win_height, SDL_WINDOW_SHOWN);
        SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        
        SDL_Texture *bg_tex = NULL;
        if (bg_surf) {
            bg_tex = SDL_CreateTextureFromSurface(ren, bg_surf);
            SDL_FreeSurface(bg_surf);
        } else {
            printf("Could not load mercurio.png (placeholder used)\n");
        }

        // Load OLED face
        SDL_Surface *oled_surf = IMG_Load("oled_module.png");
        if (oled_surf) {
            oled_face_w = oled_surf->w;
            oled_face_h = oled_surf->h;
            oled_face_tex = SDL_CreateTextureFromSurface(ren, oled_surf);
            SDL_FreeSurface(oled_surf);
        } else {
            printf("Could not load oled_module.png\n");
        }

        // Create 1x1 key texture for filled rotated rectangles
        SDL_Texture* key_tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 1, 1);
        SDL_SetTextureBlendMode(key_tex, SDL_BLENDMODE_BLEND);
        SDL_SetRenderTarget(ren, key_tex);
        SDL_SetRenderDrawColor(ren, 0, 192, 192, 178); // Light Teal, 70% opacity
        SDL_RenderClear(ren);
        SDL_SetRenderTarget(ren, NULL);

        // Start AVR thread now that GUI is ready
        pthread_create(&avr_thread, NULL, avr_run_thread, NULL);

        // GUI Event Loop
        SDL_Event e;
        while (sim_running) {
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) sim_running = 0;
                
                // Mouse interaction for PS/2
                if (e.type == SDL_MOUSEBUTTONDOWN) {
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        inject_ps2_byte(avr, 0x09); // Left Click packet
                        inject_ps2_byte(avr, 0x00);
                        inject_ps2_byte(avr, 0x00);
                    } else if (e.button.button == SDL_BUTTON_RIGHT) {
                        inject_ps2_byte(avr, 0x0A); // Right Click packet
                        inject_ps2_byte(avr, 0x00);
                        inject_ps2_byte(avr, 0x00);
                    }
                }
                if (e.type == SDL_MOUSEMOTION && (e.motion.state & SDL_BUTTON_LMASK)) {
                    // Drag to move PS/2 mouse
                    int8_t dx = e.motion.xrel;
                    int8_t dy = -e.motion.yrel; // Invert Y
                    inject_ps2_byte(avr, 0x09); // Left Click packet
                    inject_ps2_byte(avr, dx);
                    inject_ps2_byte(avr, dy);
                }
                
                // Mouse wheel for LDR
                if (e.type == SDL_MOUSEWHEEL) {
                    static int ldr_mv = 1500;
                    ldr_mv += e.wheel.y * 100;
                    if (ldr_mv < 0) ldr_mv = 0;
                    if (ldr_mv > 5000) ldr_mv = 5000;
                    inject_ldr_voltage(avr, ldr_mv);
                    printf("SIM: LDR voltage set to %dmV\n", ldr_mv);
                }
            }

            // Silence detection for Audio
            if (avr->cycle - last_piezo_toggle > 800000) { // 0.05 seconds of silence
                current_audio_freq = 0.0;
            }

            SDL_RenderClear(ren);

            // Draw Background
            if (bg_tex) {
                SDL_RenderCopy(ren, bg_tex, NULL, NULL);
            }

            // Calculate scale proportions in case window/image is resized
            float scale_x = 1.0f;
            float scale_y = 1.0f;
            if (bg_tex) {
                int win_w, win_h;
                SDL_GetWindowSize(win, &win_w, &win_h);
                int img_w, img_h;
                SDL_QueryTexture(bg_tex, NULL, NULL, &img_w, &img_h);
                if (img_w > 0 && img_h > 0) {
                    scale_x = (float)win_w / (float)img_w;
                    scale_y = (float)win_h / (float)img_h;
                }
            }
            // Draw Key Positions
            if (key_tex) {
                for (int i = 0; i < key_position_count; i++) {
                    float x = key_positions[i].x;
                    float y = key_positions[i].y;
                    float s = key_positions[i].size;
                    double a = key_positions[i].angle; // SDL_RenderCopyEx takes degrees clockwise!
                    
                    SDL_Rect dstrect = {
                        (int)(x * scale_x),
                        (int)(y * scale_y),
                        (int)(s * scale_x),
                        (int)(s * scale_y)
                    };
                    
                    // Pivot at (0, 0) means the rotation anchors precisely at the top-left (x, y) 
                    // fixing the previous right/upward shift that occurred by rotating around the center.
                    SDL_Point pivot = {0, 0};
                    SDL_RenderCopyEx(ren, key_tex, NULL, &dstrect, a, &pivot, SDL_FLIP_NONE);
                }
            }


            // Draw RGB LEDs (using coordinates from CSV)
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            for (int i = 0; i < 35; i++) {
                uint8_t green = (ws2812_colors[i] >> 16) & 0xFF; // WS2812 is GRB
                uint8_t red = (ws2812_colors[i] >> 8) & 0xFF;
                uint8_t blue = ws2812_colors[i] & 0xFF;
                
                if (red > 0 || green > 0 || blue > 0) {
                    int scaled_x = (int)(led_positions[i].x * scale_x);
                    int scaled_y = (int)(led_positions[i].y * scale_y);
                    int scaled_r = (int)(led_positions[i].radius * ((scale_x + scale_y) / 2.0f));
                    
                    SDL_SetRenderDrawColor(ren, red, green, blue, 255); // Solid color
                    SDL_RenderFillCircle(ren, scaled_x, scaled_y, scaled_r);
                }
            }

            // Draw OLED Module Face
            if (oled_face_tex) {
                SDL_Rect face_rect = {
                    (int)(oled_mod_pos_x * scale_x),
                    (int)(oled_mod_pos_y * scale_y),
                    (int)(oled_face_w * scale_x),
                    (int)(oled_face_h * scale_y)
                };
                SDL_RenderCopy(ren, oled_face_tex, NULL, &face_rect);
            }

            // Draw OLED 128x64 Pixels
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            int oled_pixels = 0;
            for (int p = 0; p < 8; p++) {
                for (int c = 0; c < 128; c++) {
                    uint8_t column_data = oled.vram[p][c];
                    for (int bit = 0; bit < 8; bit++) {
                        if (column_data & (1 << bit)) {
                            oled_pixels++;
                            int px_x1 = (int)((oled_mod_rend_x + c) * scale_x);
                            int px_y1 = (int)((oled_mod_rend_y + p * 8 + bit) * scale_y);
                            int px_x2 = (int)((oled_mod_rend_x + c + 1) * scale_x);
                            int px_y2 = (int)((oled_mod_rend_y + p * 8 + bit + 1) * scale_y);
                            SDL_Rect pixel_rect = { px_x1, px_y1, px_x2 - px_x1, px_y2 - px_y1 };
                            if (pixel_rect.w == 0) pixel_rect.w = 1;
                            if (pixel_rect.h == 0) pixel_rect.h = 1;
                            SDL_RenderFillRect(ren, &pixel_rect);
                        }
                    }
                }
            }

            static uint8_t last_oled_init = 0xFF;
            if (avr->data[0x0690] != last_oled_init) {
                last_oled_init = avr->data[0x0690];
            }

            int vram_sum = 0;
            for (int p = 0; p < 8; p++) {
                for (int c = 0; c < 128; c++) {
                    vram_sum += oled.vram[p][c];
                }
            }
            if (vram_sum > 0) {
                FILE *f = fopen("oled_dump.txt", "w");
                if (f) {
                    for (int p = 0; p < 8; p++) {
                        fprintf(f, "Page %d:\n", p);
                        for (int bit = 0; bit < 8; bit++) {
                            for (int c = 0; c < 128; c++) {
                                fprintf(f, "%c", (oled.vram[p][c] & (1 << bit)) ? '#' : ' ');
                            }
                            fprintf(f, "\n");
                        }
                    }
                    fclose(f);
                }
            }



            SDL_SetRenderDrawColor(ren, 30, 30, 30, 255);
            SDL_RenderPresent(ren);
            SDL_Delay(16); // ~60fps
        }

        if (audio_dev > 0) SDL_CloseAudioDevice(audio_dev);
        if (bg_tex) SDL_DestroyTexture(bg_tex);
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
    }

    pthread_join(avr_thread, NULL);
    avr_vcd_stop(&vcd_file);
    return 0;
}
