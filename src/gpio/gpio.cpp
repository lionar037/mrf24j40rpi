// gpio.cpp

#include <gpio/gpio.hpp>
#include <config/config.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <poll.h>

namespace GPIO {

    Gpio_t::Gpio_t(bool& st) : m_state{st} {
        settings(m_gpio_in, DIR_IN, filenameGpio);
        settings(m_gpio_out, DIR_OUT, filenameGpio);
    }

    void Gpio_t::set() {
        gpio_set_edge(m_gpio_in, EDGE_FALLING);
        #ifdef USE_MRF24_RX
        gpio_set_value(m_gpio_out, VALUE_LOW);
        #else
        gpio_set_value(m_gpio_out, VALUE_HIGH);
        #endif
    }

    // Helper functions

    int Gpio_t::file_open_and_write_value(const std::string_view fname, const std::string_view wdata) {
        std::ofstream file(fname.data(), std::ios::out | std::ios::app);
        if (!file.is_open()) {
            std::cerr << "Could not open file " << fname << "\n";
            return -1;
        }
        file.write(wdata.data(), wdata.size());
        return 0;
    }

    int Gpio_t::gpio_export(int gpio_num) {
        return file_open_and_write_value(SYSFS_GPIO_PATH SYSFS_GPIO_EXPORT_FN, std::to_string(gpio_num));
    }

    int Gpio_t::gpio_unexport(int gpio_num) {
        return file_open_and_write_value(SYSFS_GPIO_PATH SYSFS_GPIO_UNEXPORT_FN, std::to_string(gpio_num));
    }

    int Gpio_t::gpio_set_direction(int gpio_num, const std::string_view dir) {
        std::string path = std::string(SYSFS_GPIO_PATH) + "/gpio" + std::to_string(gpio_num) + "/direction";
        return file_open_and_write_value(path, dir);
    }

    int Gpio_t::gpio_set_value(int gpio_num, const std::string_view value) {
        std::string path = std::string(SYSFS_GPIO_PATH) + "/gpio" + std::to_string(gpio_num) + "/value";
        return file_open_and_write_value(path, value);
    }

    int Gpio_t::gpio_set_edge(int gpio_num, const std::string_view edge) {
        std::string path = std::string(SYSFS_GPIO_PATH) + "/gpio" + std::to_string(gpio_num) + "/edge";
        return file_open_and_write_value(path, edge);
    }

    int Gpio_t::gpio_get_fd_to_value(int gpio_num) {
        std::string path = std::string(SYSFS_GPIO_PATH) + "/gpio" + std::to_string(gpio_num) + "/value";
        return open(path.c_str(), O_RDONLY | O_NONBLOCK);
    }

    bool Gpio_t::settings(int pin, const std::string_view str_v, std::ifstream& fileTmp) {
        const std::string filePathGpio = "/sys/class/gpio/gpio" + std::to_string(pin) + "/direction";
        const std::string fNameResult = "echo " + std::to_string(pin) + " > /sys/class/gpio/export";
        
        fileTmp.open(filePathGpio);
        if (!fileTmp.is_open()) {
            const int result_output = std::system(fNameResult.c_str());
            if (result_output != 0) {
                std::cerr << "Error exporting GPIO " << pin << ".\n";
                return false;
            }
        }

        fileTmp.close();
        gpio_unexport(pin);
        gpio_export(pin);
        gpio_set_direction(pin, str_v);
        return true;
    }

    const bool Gpio_t::app(bool& flag) {
        struct pollfd fdpoll = {};
        char buf[64];

        set();
        m_gpio_in_fd = gpio_get_fd_to_value(m_gpio_in);

        if (m_state) {
            for (int m_looper = 0; m_looper < READING_STEPS; ++m_looper) {
                fdpoll.fd = m_gpio_in_fd;
                fdpoll.events = POLLPRI;

                m_res = poll(&fdpoll, 1, POLL_TIMEOUT);
                if (m_res < 0) {
                    std::cerr << "Poll failed... " << m_res << "\n";
                }
                if (m_res == 0) {
                    std::cout << "\nPoll timed out or received button press...\n";
                }
                if (fdpoll.revents & POLLPRI) {
                    lseek(fdpoll.fd, 0, SEEK_SET);
                    read(fdpoll.fd, buf, sizeof(buf));
                }
            }
        } else {
            #ifdef USE_MRF24_RX
            gpio_set_value(m_gpio_out, flag ? VALUE_HIGH : VALUE_LOW);
            #else
            gpio_set_value(m_gpio_out, VALUE_LOW);
            #endif
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        #ifdef USE_MRF24_RX
        gpio_set_value(m_gpio_out, flag ? VALUE_HIGH : VALUE_LOW);
        #else
        gpio_set_value(m_gpio_out, VALUE_HIGH);
        #endif
        return false;
    }

    void Gpio_t::CloseGpios() {
        if (filenameGpio.is_open()) filenameGpio.close();
        close(m_gpio_in_fd);
        gpio_set_value(m_gpio_out, VALUE_LOW);
        gpio_unexport(m_gpio_out);
        gpio_unexport(m_gpio_in);
    }

    Gpio_t::~Gpio_t() {
        CloseGpios();
        std::cout << "~Gpio()\n";
    }

}