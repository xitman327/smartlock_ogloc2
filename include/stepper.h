#include <inttypes.h>
#define mot_dir 32
#define mot_step 33
#define mot_en 25
#define home_sw 4
#define steps_rev 15000
#define steps_deg 30
#define degtosteps(a) (a*steps_deg)
#define stepstodeg(a) (a/steps_deg) 

byte lock_state;
enum lockStates { error, locked, unlocked, lock, unlock, home, test };
String state_str[] = {"error", "locked", "unlocked", "lock", "unlock", "home", "test"};
lockStates rotator = home;
bool home_inv;

#include "AccelStepper.h"
AccelStepper myStepper(AccelStepper::DRIVER, mot_step, mot_dir);

uint8_t state_home, state_test;
bool bsman, seccondhit;
void rotate_lock() {
  switch(rotator){
    case home:
        switch(state_home){
            case 0:
                if (digitalRead(home_sw)){
                    //stepper->move(500);
                    myStepper.move(500);
                }
                state_home = 1;
                break;
            case 1:
                if (digitalRead(home_sw) == 0 && !myStepper.isRunning()){
                    //stepper->move(15000);
                    myStepper.move(15000);
                    state_home = 2;
                }
                break;
            case 2:
                if(digitalRead(home_sw) == 1 && myStepper.isRunning() && !seccondhit){
                    myStepper.stop();
                    myStepper.setCurrentPosition(degtosteps(2*360) + 100);
                    myStepper.move(25000);
                    seccondhit = 1;
                    
                }
                if(seccondhit && myStepper.distanceToGo() < 20000){
                    state_home = 3;
                }
                if(!myStepper.isRunning()){
                    state_home = 3;
                }
                break;
            case 3:
                if(digitalRead(home_sw) == 1 && myStepper.isRunning()){
                    //stepper->forceStop();
                    myStepper.stop();
                    if(myStepper.currentPosition() > 20000){
                        rotator = locked;
                        if(home_inv){
                            myStepper.setCurrentPosition(degtosteps(2*360) + 100);
                        }else{
                            myStepper.setCurrentPosition(degtosteps(2*360) - 100);
                        }
                    }else{
                        rotator = locked;
                        if(home_inv){
                            myStepper.setCurrentPosition(degtosteps(2*360) + 100);
                        }else{
                            myStepper.setCurrentPosition(degtosteps(2*360) - 100);
                        }
                    }state_home = 3;
                    state_home = 3;

                }else if(digitalRead(home_sw) == 0 && !myStepper.isRunning()){
                    myStepper.move(-15000);
                    home_inv = 1;
                }
                break;
            default:
                break;
        }
        
      break;
      
    case lock:
        // if(rotator != locked){
        //     //stepper->moveTo(degtosteps(2*360));
        //     myStepper.moveTo(degtosteps(2*360));
        //     rotator = locked;
        // }
        if(rotator != locked){
            if(!myStepper.isRunning() && !bsman){
                myStepper.moveTo(degtosteps(2*360));
                bsman = 1;
            }else
            if(digitalRead(home_sw) && myStepper.distanceToGo()< degtosteps(270) && bsman){
                myStepper.stop();
                myStepper.setCurrentPosition(degtosteps(2*360));
                rotator = locked;
                bsman = 0;
            }else
            if(!digitalRead(home_sw) && !myStepper.isRunning() && bsman){
                myStepper.move(5000);
            }
        }
    break;

    case unlock:
        // if(rotator != unlocked){
        //     //stepper->move(degtosteps(1*360));
        //     myStepper.moveTo(degtosteps(1*360));
        //     rotator = unlocked;
        // }
        if(rotator != unlocked){
            if(!myStepper.isRunning() && !bsman){
                myStepper.moveTo(degtosteps(1*360));
                bsman = 1;
            }else
            if(digitalRead(home_sw) && ~myStepper.distanceToGo() < degtosteps(270) && bsman){
                myStepper.stop();
                myStepper.setCurrentPosition(degtosteps(1*360));
                rotator = unlocked;
                bsman = 0;
            }else
            if(!digitalRead(home_sw) && !myStepper.isRunning() && bsman){
                myStepper.move(-5000);
            }
        }
      break;

    default:
        break;
        
    case test:
        switch (state_test){
            case 0:
                if(digitalRead(home_sw) && !myStepper.isRunning()){
                    myStepper.move(500);
                }
                if(!myStepper.isRunning()){
                    state_test++;
                }
                break;
            case 1:
                if(!digitalRead(home_sw) && !myStepper.isRunning()){
                    myStepper.move(-500);
                }else
                if(digitalRead(home_sw) && myStepper.isRunning()){
                    myStepper.stop();
                    myStepper.setCurrentPosition(0);
                    state_test++;
                }
                break;
            case 2:
                if(digitalRead(home_sw) && !myStepper.isRunning()){
                    myStepper.move(10000);
                }else
                if(digitalRead(home_sw) && myStepper.isRunning() && myStepper.currentPosition() > 500){
                    myStepper.stop();
                    Serial.print('\n' + myStepper.currentPosition() + '\n');
                    //rotator = lock;
                    state_test++;
                }
                break;
            default:
                break;
        }
        break;
  }
}

void stepper_init(){
    myStepper.setMaxSpeed(13000);
    myStepper.setAcceleration(50000);
    myStepper.setSpeed(15000);
    //myStepper.setMinPulseWidth(20);
    myStepper.setEnablePin(mot_en);
    myStepper.setPinsInverted(0,0,1);
    //myStepper.enableOutputs();
}