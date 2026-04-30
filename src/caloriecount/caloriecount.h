#ifndef CALORIECOUNT_H
#define CALORIECOUNT_H


class Caloriecount {
public:

  float getBPM;
  float getPace;
  float askWeight;
  float askHeight;

  int getSteps;
  int askAge;

  void userdatagather();
  void begin();

private:

  bool ageAsked = false;
  bool weightAsked = false;
  bool heightAsked = false;
  bool questionsAsked = false;
  unsigned long currentTime = 0;
  unsigned long pastTime = 0;

};


#endif