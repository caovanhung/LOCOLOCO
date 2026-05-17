#include "wled.h"

/*
 * LED methods
 */

void setValuesFromMainSeg()          { setValuesFromSegment(strip.getMainSegmentId()); }
void setValuesFromFirstSelectedSeg() { setValuesFromSegment(strip.getFirstSelectedSegId()); }
void setValuesFromSegment(uint8_t s)
{
  Segment& seg = strip.getSegment(s);
  colPri[0] = R(seg.colors[0]);
  colPri[1] = G(seg.colors[0]);
  colPri[2] = B(seg.colors[0]);
  colPri[3] = W(seg.colors[0]);
  colSec[0] = R(seg.colors[1]);
  colSec[1] = G(seg.colors[1]);
  colSec[2] = B(seg.colors[1]);
  colSec[3] = W(seg.colors[1]);
  effectCurrent   = seg.mode;
  effectSpeed     = seg.speed;
  effectIntensity = seg.intensity;
  effectPalette   = seg.palette;
}


// applies global legacy values (col, colSec, effectCurrent...)
// problem: if the first selected segment already has the value to be set, other selected segments are not updated
void applyValuesToSelectedSegs()
{
  // copy of first selected segment to tell if value was updated
  unsigned firstSel = strip.getFirstSelectedSegId();
  Segment selsegPrev = strip.getSegment(firstSel);
  for (unsigned i = 0; i < strip.getSegmentsNum(); i++) {
    Segment& seg = strip.getSegment(i);
    if (i != firstSel && (!seg.isActive() || !seg.isSelected())) continue;

    if (effectSpeed     != selsegPrev.speed)     {seg.speed     = effectSpeed;     stateChanged = true;}
    if (effectIntensity != selsegPrev.intensity) {seg.intensity = effectIntensity; stateChanged = true;}
    if (effectPalette   != selsegPrev.palette)   {seg.setPalette(effectPalette);}
    if (effectCurrent   != selsegPrev.mode)      {seg.setMode(effectCurrent);}
    uint32_t col0 = RGBW32(colPri[0], colPri[1], colPri[2], colPri[3]);
    uint32_t col1 = RGBW32(colSec[0], colSec[1], colSec[2], colSec[3]);
    if (col0 != selsegPrev.colors[0])            {seg.setColor(0, col0);}
    if (col1 != selsegPrev.colors[1])            {seg.setColor(1, col1);}
  }
}


void toggleOnOff()
{
  #ifdef WLED_DEBUG
  DEBUG_PRINTF_P(PSTR("toggleOnOff: %d\n"), bri);
  #endif
  if (bri == 0)
  {
    bri = briLast;
    strip.restartRuntime();
  } else
  {
    briLast = bri;
    bri = 0;
  }
  stateChanged = true;
}


//scales the brightness with the briMultiplier factor
byte scaledBri(byte in)
{
  unsigned val = ((uint16_t)in*briMultiplier)/100;
  if (val > 255) val = 255;
  return (byte)val;
}


//applies global temporary brightness (briT) to strip
void applyBri() {
  if (!(realtimeMode && arlsForceMaxBri)) {
    //DEBUG_PRINTF_P(PSTR("Applying strip brightness: %d (%d,%d)\n"), (int)briT, (int)bri, (int)briOld);
    strip.setBrightness(scaledBri(briT));
  }
}


//applies global brightness and sets it as the "current" brightness (no transition)
void applyFinalBri() {
  briOld = bri;
  briT = bri;
  applyBri();
  strip.trigger(); // force one last update
}


//called after every state changes, schedules interface updates, handles brightness transition and nightlight activation
//unlike colorUpdated(), does NOT apply any colors or FX to segments
void stateUpdated(uint8_t callMode) {
  DEBUG_PRINTF_P(PSTR("stateUpdated() called with mode %d\n"), callMode);
  
  if (callMode == CALL_MODE_DIRECT_CHANGE) {
    DEBUG_PRINTLN(F("Direct change mode - updating LED state"));
    
    // Get values from first selected segment
    Segment& seg = strip.getSegment(0);
    uint8_t segBri = seg.getBrightness();
    bool segState = seg.isActive();
    
    DEBUG_PRINTF_P(PSTR("Selected segment: bri=%d, state=%d\n"), segBri, segState);
    
    // Update global state
    if (bri != segBri) {
      bri = segBri;
      if (bri) briLast = bri;
    }
    
    if (state != segState) {
      state = segState;
      DEBUG_PRINTF_P(PSTR("State changed to %d\n"), state);
    }

    // Reset segment runtime if brightness or state changed
    if (bri != briOld || state != stateOld) {
      DEBUG_PRINTLN(F("Brightness or state changed - resetting segments"));
      strip.resetSegments();
      if (currentPreset != 0) {
        DEBUG_PRINTLN(F("Resetting current preset"));
        currentPreset = 0;
      }
    }

    // Handle nightlight
    if (nightlightActive && !nightlightActiveOld) {
      DEBUG_PRINTLN(F("Nightlight activated"));
      nightlightActiveOld = true;
      nightlightStartTime = millis();
    } else if (!nightlightActive && nightlightActiveOld) {
      DEBUG_PRINTLN(F("Nightlight deactivated"));
      nightlightActiveOld = false;
    }

    // Update interface call mode
    if (callMode != CALL_MODE_NO_NOTIFY) {
      DEBUG_PRINTLN(F("Notifying interfaces of state change"));
      notify(callMode);
    }

    // Handle brightness and transition
    if (bri != briT) {
      DEBUG_PRINTF_P(PSTR("Brightness transition: %d -> %d\n"), briT, bri);
      if (bri > briT) {
        briT++;
      } else {
        briT--;
      }
    }

    if (transitionActive) {
      DEBUG_PRINTLN(F("Transition active - updating transition"));
      transitionDelay = transitionDelay;
      transitionActive = false;
      transitionStartTime = millis();
    }

    // Reset timebase if brightness target is 0
    if (briT == 0) {
      DEBUG_PRINTLN(F("Brightness target is 0 - resetting timebase"));
      strip.resetTimebase();
    }

    // Notify usermods of state change
    usermods.handleStateChange(callMode);

    // Handle transitions
    if (transitionActive) {
      DEBUG_PRINTLN(F("Processing transition"));
      handleTransitions();
    }
  }
}


void updateInterfaces(uint8_t callMode) {
  if (!interfaceUpdateCallMode || millis() - lastInterfaceUpdate < INTERFACE_UPDATE_COOLDOWN) return;

  // sendDataWs();
  // lastInterfaceUpdate = millis();
  // interfaceUpdateCallMode = CALL_MODE_INIT; //disable further updates

  // if (callMode == CALL_MODE_WS_SEND) return;

  // #ifndef WLED_DISABLE_ALEXA
  // if (espalexaDevice != nullptr && callMode != CALL_MODE_ALEXA) {
  //   espalexaDevice->setValue(bri);
  //   espalexaDevice->setColor(colPri[0], colPri[1], colPri[2]);
  // }
  // #endif
  #ifndef WLED_DISABLE_MQTT
  publishMqtt();
  #endif
}


void handleTransitions() {
  //handle still pending interface update
  updateInterfaces(interfaceUpdateCallMode);

  if (transitionActive && strip.getTransition() > 0) {
    int ti = millis() - transitionStartTime;
    int tr = strip.getTransition();
    if (ti/tr) {
      strip.setTransitionMode(false); // stop all transitions
      // restore (global) transition time if not called from UDP notifier or single/temporary transition from JSON (also playlist)
      if (jsonTransitionOnce) strip.setTransition(transitionDelay);
      transitionActive = false;
      jsonTransitionOnce = false;
      applyFinalBri();
      return;
    }
    byte briTO = briT;
    int deltaBri = (int)bri - (int)briOld;
    briT = briOld + (deltaBri * ti / tr);
    if (briTO != briT) applyBri();
  }
}


// legacy method, applies values from col, effectCurrent, ... to selected segments
void colorUpdated(byte callMode) {
  applyValuesToSelectedSegs();
  stateUpdated(callMode);
}


void handleNightlight() {
  unsigned long now = millis();
  if (now < 100 && lastNlUpdate > 0) lastNlUpdate = 0; // take care of millis() rollover
  if (now - lastNlUpdate < 100) return; // allow only 10 NL updates per second
  lastNlUpdate = now;

  if (nightlightActive)
  {
    if (!nightlightActiveOld) //init
    {
      nightlightStartTime = millis();
      nightlightDelayMs = (unsigned)(nightlightDelayMins*60000);
      nightlightActiveOld = true;
      briNlT = bri;
      for (unsigned i=0; i<4; i++) colNlT[i] = colPri[i]; // remember starting color
      if (nightlightMode == NL_MODE_SUN)
      {
        //save current
        colNlT[0] = effectCurrent;
        colNlT[1] = effectSpeed;
        colNlT[2] = effectPalette;

        strip.getFirstSelectedSeg().setMode(FX_MODE_STATIC); // make sure seg runtime is reset if it was in sunrise mode
        effectCurrent = FX_MODE_SUNRISE;            // colorUpdated() will take care of assigning that to all selected segments
        effectSpeed = nightlightDelayMins;
        effectPalette = 0;
        if (effectSpeed > 60) effectSpeed = 60; //currently limited to 60 minutes
        if (bri) effectSpeed += 60; //sunset if currently on
        briNlT = !bri; //true == sunrise, false == sunset
        if (!bri) bri = briLast;
        colorUpdated(CALL_MODE_NO_NOTIFY);
      }
    }
    float nper = (millis() - nightlightStartTime)/((float)nightlightDelayMs);
    if (nightlightMode == NL_MODE_FADE || nightlightMode == NL_MODE_COLORFADE)
    {
      bri = briNlT + ((nightlightTargetBri - briNlT)*nper);
      if (nightlightMode == NL_MODE_COLORFADE)                                         // color fading only is enabled with "NF=2"
      {
        for (unsigned i=0; i<4; i++) colPri[i] = colNlT[i]+ ((colSec[i] - colNlT[i])*nper);   // fading from actual color to secondary color
      }
      colorUpdated(CALL_MODE_NO_NOTIFY);
    }
    if (nper >= 1) //nightlight duration over
    {
      nightlightActive = false;
      if (nightlightMode == NL_MODE_SET)
      {
        bri = nightlightTargetBri;
        colorUpdated(CALL_MODE_NO_NOTIFY);
      }
      if (bri == 0) briLast = briNlT;
      if (nightlightMode == NL_MODE_SUN)
      {
        if (!briNlT) { //turn off if sunset
          effectCurrent = colNlT[0];
          effectSpeed = colNlT[1];
          effectPalette = colNlT[2];
          toggleOnOff();
          applyFinalBri();
        }
      }

      if (macroNl > 0)
        applyPreset(macroNl);
      nightlightActiveOld = false;
    }
  } else if (nightlightActiveOld) //early de-init
  {
    if (nightlightMode == NL_MODE_SUN) { //restore previous effect
      effectCurrent = colNlT[0];
      effectSpeed = colNlT[1];
      effectPalette = colNlT[2];
      colorUpdated(CALL_MODE_NO_NOTIFY);
    }
    nightlightActiveOld = false;
  }
}

//utility for FastLED to use our custom timer
uint32_t get_millisecond_timer() {
  return strip.now;
}
