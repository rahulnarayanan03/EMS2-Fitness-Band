#include <Arduino.h>
#include "caloriecount.h"

void begin() {
    Serial.begin(9600);
    unsigned long currentTime = millis();

};

void Caloriecount::userdatagather() {

if(!questionsAsked) {

  bool ageAsked = false;
  bool weightAsked = false;
  bool heightAsked = false;
  
  if(currentTime > 1000) {
  bool questionsAsked = true;

  }
}   

 if(questionsAsked){

  Serial.println("Please specify your age, height and weight");
  
  if(Serial.available() > 0) {
    Serial.readString();
    
  }




 }
};