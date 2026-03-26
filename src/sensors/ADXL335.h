#ifndef ADXL335_H
#define ADXL335_H

class ADXL335 {
public:
    ADXL335(int pinX, int pinY, int pinZ);

    void begin();
    
    //For reading raw acceleration from X, Y, Z axes via accelerometer 
    void readAcc(float &x, float &y, float &z);

private:
    int _pinX;
    int _pinY;
    int _pinZ;

};

#endif