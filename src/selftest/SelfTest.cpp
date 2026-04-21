#include <Arduino.h>
#include "SelfTest.h"
SelfTest::SelfTest(Calibration &cal, int stPin)
   : _cal(cal), _stPin(stPin) {}
bool SelfTest::run() {
   Serial.println("[ST] Starting ADXL335 self test...");
   pinMode(_stPin, OUTPUT);
   digitalWrite(_stPin, LOW);
   delay(100);
   // sample baseline with ST low
   float baseX = _cal.getXG();
   float baseY = _cal.getYG();
   float baseZ = _cal.getZG();
   Serial.print("[ST] Baseline X: "); Serial.print(baseX, 3);
   Serial.print(" Y: "); Serial.print(baseY, 3);
   Serial.print(" Z: "); Serial.println(baseZ, 3);
   // pull ST high - applies electrostatic force to beam
   digitalWrite(_stPin, HIGH);
   delay(200); // let it settle
   float stX = _cal.getXG();
   float stY = _cal.getYG();
   float stZ = _cal.getZG();
   // pull ST low again
   digitalWrite(_stPin, LOW);
   // calculate deltas
   _deltaX = stX - baseX;
   _deltaY = stY - baseY;
   _deltaZ = stZ - baseZ;
   Serial.print("[ST] Delta X: "); Serial.print(_deltaX, 3);
   Serial.print(" Y: "); Serial.print(_deltaY, 3);
   Serial.print(" Z: "); Serial.println(_deltaZ, 3);
   // check against expected values with tolerance
   bool passX = (_deltaX < 0) && (abs(_deltaX) > abs(ST_X_EXPECTED) * (1.0f - ST_TOLERANCE));
   bool passY = (_deltaY > 0) && (_deltaY > ST_Y_EXPECTED * (1.0f - ST_TOLERANCE));
   bool passZ = (_deltaZ > 0) && (_deltaZ > ST_Z_EXPECTED * (1.0f - ST_TOLERANCE));
   if (!passX) {
       _result = STResult::FAIL_X;
       Serial.println("[ST] FAIL — X axis out of range");
   } else if (!passY) {
       _result = STResult::FAIL_Y;
       Serial.println("[ST] FAIL — Y axis out of range");
   } else if (!passZ) {
       _result = STResult::FAIL_Z;
       Serial.println("[ST] FAIL — Z axis out of range");
   } else {
       _result = STResult::PASS;
       Serial.println("[ST] PASS — all axes within tolerance");
   }
   return (_result == STResult::PASS);
}
STResult SelfTest::getResult() { return _result; }
float SelfTest::getDeltaX() { return _deltaX; }
float SelfTest::getDeltaY() { return _deltaY; }
float SelfTest::getDeltaZ() { return _deltaZ; }
const char* SelfTest::getResultStr() {
   switch (_result) {
       case STResult::PASS:    return "PASS";
       case STResult::FAIL_X:  return "FAIL X";
       case STResult::FAIL_Y:  return "FAIL Y";
       case STResult::FAIL_Z:  return "FAIL Z";
       case STResult::NOT_RUN: return "NOT RUN";
       default:                return "UNKNOWN";
   }
}