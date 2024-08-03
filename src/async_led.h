#ifndef ASYNC_LED_H
#define ASYNC_LED_H
#include <unistd.h>
#include <Arduino.h>
#include <list>
#include <Ticker.h>

class AsyncLed {
public:
    AsyncLed(uint8_t pin, bool invertLogic=false) : pin{pin}, invertLogic{invertLogic} {
   }

    static void SetupLeds(std::initializer_list<AsyncLed*> _leds) {
        for(auto led: _leds) {
            AsyncLed::leds.push_back(led);
            led->setup();
        }
        ticker.attach_ms(100, [](){
            // Serial.println("Tick");
            for(auto led: leds) {
                led->loop();
            }
        });
        // Serial.printf("Ticker active: %d\n", ticker.active());  
    }

    void on() {
        steps = 0;
        writePin(true);
    }

    void off() {
        steps = 0;
        writePin(false);
    }

    // Each "blink" is 100ms
    void setPattern(uint8_t onBlinks, uint8_t offBlinks, uint8_t steps=2, uint8_t blinksBetweenRepeats=0, bool stateBetweenRepeats = false) {
        this->onBlinks = onBlinks;
        this->offBlinks = offBlinks;
        this->steps = steps;
        this->blinksBetweenRepeats = blinksBetweenRepeats;
        this->blinksToNextStep = onBlinks;
        this->step = 0;
        this->stateBetweenRepeats = stateBetweenRepeats;
    }

    void setDuty(float duty) { this->duty = duty; }

private:
    uint8_t pin;
    bool invertLogic;
    uint8_t onBlinks;
    uint8_t offBlinks;
    uint8_t steps;
    uint8_t blinksBetweenRepeats;

    uint8_t blinksToNextStep;
    uint8_t step;
    bool stateBetweenRepeats;
    float duty = 1;


    

    void writePin(bool on) {
        if(duty != 1) {
            analogWrite(pin, invertLogic != on ? duty * 255: 0);
        } else {
            digitalWrite(pin, invertLogic != on ? HIGH : LOW);
        }
    }

    void setup() {
        pinMode(pin, OUTPUT);
        writePin(false);
        leds.push_back(this);
    }


    void loop() {
        if(steps == 0) {
            return;
        }
        bool state = (step == steps) ? stateBetweenRepeats : (step%2 == 0);
        // Serial.printf("LED loop: steps=%d, blinksToNextStep=%d, step=%d, on=%d\n", steps, blinksToNextStep, step, state);

        if(blinksToNextStep > 0) {
            blinksToNextStep--;
            writePin(state);
            return;
        }
        step = (step+1) % (steps+1);

        blinksToNextStep = !state ? onBlinks : offBlinks;
        if(step == steps) {
            blinksToNextStep = blinksBetweenRepeats;
        }

    }
    static inline Ticker ticker;
    static inline std::list<AsyncLed*> leds;
};

#endif
