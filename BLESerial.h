#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

class BLESerialClass : public BLEServerCallbacks, public BLECharacteristicCallbacks {
    typedef std::function<void(uint8_t* data, size_t len)> DataCb;
    typedef std::function<void(bool connected)> StateCb;

   public:
    // запустить
    void begin(const char* name, uint16_t maxTX = 128, uint16_t serviceUUID = 0xFFE0, uint16_t charxUUID = 0xFFE1) {
        _state = false;
        _max_tx = maxTX;
        BLEDevice::init(name);

        BLEServer* pServer = BLEDevice::createServer();
        pServer->setCallbacks(this);

        BLEService* pService = pServer->createService(serviceUUID);
        _charx = pService->createCharacteristic(
            charxUUID,
            BLECharacteristic::PROPERTY_READ |
                BLECharacteristic::PROPERTY_WRITE |
                BLECharacteristic::PROPERTY_NOTIFY);
        _charx->addDescriptor(new BLE2902());
        _charx->setCallbacks(this);
        pService->start();

        // BLEAdvertising* pAdv = BLEDevice::getAdvertising();
        // pAdv->addServiceUUID(serviceUUID);
        // pAdv->setScanResponse(false);
        // pAdv->setMinPreferred(0x0);
        // BLEDevice::startAdvertising();

        BLEAdvertising* pAdv = pServer->getAdvertising();
        pAdv->addServiceUUID(serviceUUID);
        pAdv->setScanResponse(false);
        pAdv->setMinPreferred(0x00);
        pAdv->start();
    }

    // остановить
    void end() {
        BLEDevice::deinit(true);
        _charx = nullptr;
        _state = false;
    }

    // отправить
    void send(uint8_t* data, size_t len) {
        if (!_charx || !_state) return;

        while (len) {
            size_t tlen = (len < _max_tx) ? len : _max_tx;
            _charx->setValue(data, tlen);
            _charx->notify();
            data += tlen;
            len -= tlen;
        }
    }
    void send(const String& data) {
        send((uint8_t*)data.c_str(), data.length());
    }

    // обработчик данных вида f(uint8_t* data, size_t len)
    void onData(DataCb cb) {
        _data_cb = cb;
    }

    // обработчик смены подключения вида f(bool state)
    void onState(StateCb cb) {
        _state_cb = cb;
    }

    // состояние подключения
    bool getState() {
        return _state;
    }

   private:
    BLECharacteristic* _charx = nullptr;
    DataCb _data_cb = nullptr;
    StateCb _state_cb = nullptr;
    uint16_t _max_tx;
    bool _state = false;

    void onConnect(BLEServer* ps) override {
        _state = true;
        if (_state_cb) _state_cb(true);
    }

    void onDisconnect(BLEServer* ps) override {
        _state = false;
        if (_state_cb) _state_cb(false);
        ps->getAdvertising()->start();
    }

    void onWrite(BLECharacteristic* ch) override {
        if (!_data_cb) return;
        std::string val = std::string(ch->getValue().c_str());
        _data_cb((uint8_t*)val.data(), val.size());
    }
};

extern BLESerialClass BLESerial;
