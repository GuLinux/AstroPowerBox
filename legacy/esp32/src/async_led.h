#ifndef ASYNC_LED_H
#define ASYNC_LED_H
#include <unistd.h>
#include <Arduino.h>
#include <list>
#include <Ticker.h>
#include <TaskSchedulerDeclarations.h>

class AsyncLed {
public:
    AsyncLed(uint8_t pin, bool invertLogic=false) : pin{pin}, invertLogic{invertLogic} {
   }

    void setup(Scheduler *scheduler) {
        pinMode(pin, OUTPUT);
        writePin(false);
        leds.push_back(this);
        AsyncLed::setupTask(scheduler);
    }

    float duty() const {
        return _duty;
    }

    static void setupTask(Scheduler *scheduler) {
        if(task) {
            return;
        }
        task = new Task(100, TASK_FOREVER, [](){
            // Serial.println("Task running");
            for(auto led: leds) {
                led->loop();
            }
        }, scheduler, true);
        // Serial.printf("Ticker active: %d\n", ticker.active());  
    }

    void on() {
        repeats = 0;
        writePin(true);
    }

    void off() {
        repeats = 0;
        writePin(false);
    }

    // Each "blink" is 100ms
    void setPattern(uint8_t onBlinks, uint8_t offBlinks, uint8_t repeats=1, uint8_t blinksBetweenRepeats=0, bool stateBetweenRepeats = false) {
        this->onBlinks = onBlinks;
        this->offBlinks = offBlinks;
        this->repeats = repeats;
        this->blinksBetweenRepeats = blinksBetweenRepeats;
        this->blinksToNextStep = onBlinks;
        this->step = 0;
        this->stateBetweenRepeats = stateBetweenRepeats;
    }

    void setDuty(float duty) { this->_duty = duty; }

private:
    uint8_t pin;
    bool invertLogic;
    uint8_t onBlinks;
    uint8_t offBlinks;
    uint8_t repeats;
    uint8_t blinksBetweenRepeats;

    uint8_t blinksToNextStep;
    uint8_t step;
    bool stateBetweenRepeats;
    float _duty = 1;


    

    void writePin(bool on) {
        if(_duty != 1) {
            analogWrite(pin, invertLogic != on ? _duty * 255: 0);
        } else {
            digitalWrite(pin, invertLogic != on ? HIGH : LOW);
        }
    }

    void loop() {
        if(repeats == 0) {
            return;
        }
        uint8_t steps = 2 * repeats;
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
    static inline Task *task = nullptr;
    static inline std::list<AsyncLed*> leds;
};

#endif
