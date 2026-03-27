#ifndef PACEFIND_H
#define PACEFIND_H

class PACEFIND {
public:
    void update(unsigned long currentTime);
    const char* getPace();


private:
    unsigned long lastStepTime = 0;
    const char* pace = "STANDING"; //default 

protected:

};

#endif