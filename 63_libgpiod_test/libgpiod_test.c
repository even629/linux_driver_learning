#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

static volatile int running = 1;

void ctrl_c_handler(int sig)
{
        running = 0;
}

int main(void)
{
        int ret = 0;
        unsigned int offset = 15;

        struct gpiod_chip *chip = NULL;
        struct gpiod_line_info *line_info = NULL;
        struct gpiod_line_settings *setting = NULL;
        struct gpiod_line_config *line_cfg = NULL;
        struct gpiod_request_config *req_cfg = NULL;
        struct gpiod_line_request *req = NULL;

        signal(SIGINT, ctrl_c_handler);        

        // 获取 gpiod_chip
        chip = gpiod_chip_open("/dev/gpiochip0");
        if (!chip) {
                perror("chip open");
                ret = -1;
                goto err_chip;
        }

        // 获取 gpiod_line_info
        line_info = gpiod_chip_get_line_info(chip, offset);
        if (!line_info) {
                perror("line info");
                ret = -1;
                goto err_line_info;
        }

        // 创建 gpiod_line_settings
        setting = gpiod_line_settings_new();
        if (!setting) {
                perror("settings");
                ret = -1;
                goto err_settings;
        }

        gpiod_line_settings_set_direction(setting, GPIOD_LINE_DIRECTION_OUTPUT);

        // 创建 gpiod_line_config
        line_cfg = gpiod_line_config_new();
        if (!line_cfg) {
                perror("line config");
                ret = -1;
                goto err_line_cfg;
        }

        gpiod_line_config_add_line_settings(line_cfg, &offset, 1, setting);

        // 创建 gpiod_request_config
        req_cfg = gpiod_request_config_new();
        if (!req_cfg) {
                perror("request config");
                ret = -1;
                goto err_req_cfg;
        }

        gpiod_request_config_set_consumer(req_cfg, "led-gpio15");

        // 获取 gpiod_request
        req = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
        if (!req) {
                perror("request");
                ret = -1;
                goto err_req;
        }

        printf("Blinking... Ctrl+C to exit\n");

        while (running) {
                // 使用 gpiod_request 设置 value
                gpiod_line_request_set_value(req, offset, GPIOD_LINE_VALUE_ACTIVE);
                sleep(1);
                gpiod_line_request_set_value(req, offset, GPIOD_LINE_VALUE_INACTIVE);
                sleep(1);
        }

err_req:
        if (req)
                gpiod_line_request_release(req);
err_req_cfg:
        if (req_cfg)
                gpiod_request_config_free(req_cfg);
err_line_cfg:
        if (line_cfg)
                gpiod_line_config_free(line_cfg);
err_settings:
        if (setting)
                gpiod_line_settings_free(setting);
err_line_info:
        if (line_info)
                gpiod_line_info_free(line_info);
err_chip:
        if (chip)
                gpiod_chip_close(chip);

        return ret;
}
