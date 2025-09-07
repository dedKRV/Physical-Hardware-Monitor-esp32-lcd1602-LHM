# Physical-Hardware-Monitor-esp32-lcd1602-LHM
Физический хардвейр монитор на лсд 1602 и есп32, LHM и промежуточным сервером

Вывод всех необходимых вам данных с [LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor) на лсд1602 с помощью есп32

Для начала вам нужно скачать [промежуточный сервер](https://github.com/TonTon-Macout/web-server-for-Libre-Hardware-Monitor) и настроить его под ваши данные. Далее поменять порядок парсинга на ваш и поменять значения минимум и максимум на ваши для прогресс бара(если хотите оставить режимы вывода значений как у меня) залить на вашу плату есп32. Если хотите поменять можете спокойно изменитиь режимы и названия.

Необходимые компоненты:

ESP32 WROOM32 CP9102X(или другие ЕСП32)

LCD1602(I2C)

RESISTOR 10K(также можно использывать встроеный подтягивающий резистор)

BUTTON

Схема подключения:

LCD 1602 (I2C)    ESP32
-----------------------------
VCC          →   3.3V

GND          →   GND

SDA          →   GPIO21

SCL          →   GPIO22

BUTTON
-----------------------------
GPIO12 → BUTTON → RESISTOR 10K → GND

![photo_2025-09-07_16-24-09](https://github.com/user-attachments/assets/4a779daa-26ca-4ad9-85e3-ed335b2623d3)

1.РЕЖИМ(Дата и время):

![photo_2025-09-07_16-23-51](https://github.com/user-attachments/assets/e327f6b0-d62a-4360-9f55-298586c23103)

2.РЕЖИМ(Температуры CPU и GPU):

![photo_2025-09-07_16-23-51 (2)](https://github.com/user-attachments/assets/4961e117-262f-491e-aa26-f570b4a812b8)

3.РЕЖИМ(загруженности CPU и GPU):

![photo_2025-09-07_16-23-51 (3)](https://github.com/user-attachments/assets/4fe403db-0a41-46d7-9a35-db3be6002171)

4.РЕЖИМ(Скорости вентеляторов CPU и GPU):

![photo_2025-09-07_16-23-51 (4)](https://github.com/user-attachments/assets/8933c28d-9cab-4125-9ab8-d2b8a2e24187)

5.РЕЖИМ(Upload speed и Download speed интернета):

![photo_2025-09-07_16-23-52](https://github.com/user-attachments/assets/050e95aa-1c5e-41ed-b91a-d4489f1ff04f)

Я еще изучаю ардуино, поэтому буду очень благодарен за обратную связь.
