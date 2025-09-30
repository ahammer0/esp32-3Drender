  

# ESP32 serial port
/dev/ttyACM0

#le flash
esptool.py --chip esp32c6 --port /dev/ttyUSB0 --baud 460800 write_flash -z 0x0 ESP32_GE---met la bonne version--.bin

# acces au repl
```bash
screen /dev/ttyACM0 115200
```
#testt du fw
```python
import esp
esp.check_fw()
```

# Platformio
[getting started](https://docs.platformio.org/en/latest/core/installation/methods/installer-script.html)
