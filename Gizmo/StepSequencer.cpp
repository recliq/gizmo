////// Copyright 2016 by Sean Luke
////// Licensed under the Apache 2.0 License

#include "All.h"


#if defined (__AVR_ATmega2560__)
#define __FOO__
#endif



#if defined(__FOO__) 
// Used by GET_TRACK_LENGTH to return the length of tracks in the current format
GLOBAL static uint8_t _trackLength[5] = {16, 24, 32, 48, 64};
// Used by GET_NUM_TRACKS to return the number of tracks in the current format
GLOBAL static uint8_t _numTracks[5] = {12, 8, 6, 4, 3};
#else
// Used by GET_TRACK_LENGTH to return the length of tracks in the current format
GLOBAL static uint8_t _trackLength[3] = {16, 24, 32};
// Used by GET_NUM_TRACKS to return the number of tracks in the current format
GLOBAL static uint8_t _numTracks[3] = {12, 8, 6};
#endif


void resetTrack(uint8_t track)
    {
    uint8_t trackLen = GET_TRACK_LENGTH();
    memset(&data.slot.data.stepSequencer.buffer[trackLen * track * 2], 0, trackLen * 2);
    local.stepSequencer.outMIDI[track] = MIDI_OUT_DEFAULT;  // default
    local.stepSequencer.noteLength[track] = PLAY_LENGTH_USE_DEFAULT;
    local.stepSequencer.muted[track] = 0;
    local.stepSequencer.velocity[track] = STEP_SEQUENCER_NO_OVERRIDE_VELOCITY;
    local.stepSequencer.fader[track] = FADER_IDENTITY_VALUE;
    local.stepSequencer.offTime[track] = 0;
    local.stepSequencer.noteOff[track] = NO_NOTE;
    }


// If the point holds a cursor, blinks it, else sets it.  Used to simplify
// and reduce code size
void blinkOrSetPoint(unsigned char* led, uint8_t x, uint8_t y, uint8_t isCursor)
    {
    if (isCursor)
        blinkPoint(led, x, y);
    else
        setPoint(led, x, y);
    }

// Draws the sequence with the given track length, number of tracks, and skip size
void drawStepSequencer(uint8_t trackLen, uint8_t numTracks, uint8_t skip)
    {
    clearScreen();
    
    // revise LASTTRACK to be just beyond the last track we'll draw
    //      (where TRACK is the first track we'll draw)     
        
    // this code is designed to allow the user to move down to about the middle of the screen,
    // at which point the cursor stays there and the screen scrolls instead.
    uint8_t firstTrack = local.stepSequencer.currentTrack;
//#if defined(__FOO__) 
    uint8_t fourskip =  4 / skip;
//#else
//    uint8_t fourskip = (4 >> (skip - 1));  // 4 >> (skip - 1) is a fancy way of saying 4 / skip for skip = 1 or 2.  We want to move to the middle of the screen, and only then start scrolling
//#endif
    if (firstTrack < fourskip)  
        firstTrack = 0;
    else firstTrack = firstTrack - fourskip + 1;
    
    uint8_t lastTrack = numTracks;          // lastTrack is 1+ the final track we'll be drawing
//#if defined(__FOO__) 
    uint8_t sixskip = 6 / skip;
//#else
//    uint8_t sixskip = (6 >> (skip - 1));
//#endif
    lastTrack = bound(lastTrack, 0, firstTrack + sixskip);

    // Now we start drawing each of the tracks.  We will make blinky lights for beats or for the cursor
    // and will have solid lights or nothing for the notes or their absence.
        
    uint8_t y = 7;
    for(uint8_t t = firstTrack; t < lastTrack; t++)  // for each track from top to bottom
        {
        // data is stored per-track as
        // NOTE VEL
        // We need to strip off the high bit because it's used for other packing
                
        // for each note in the track
        for (uint8_t d = 0; d < trackLen; d++)
            {
            uint8_t weAreNotSolo = (local.stepSequencer.solo && t != local.stepSequencer.currentTrack);
            uint16_t pos = (t * (uint16_t) trackLen + d) * 2;
            uint8_t vel = data.slot.data.stepSequencer.buffer[pos + 1];
            // check for tie
            if ((vel == 0) && (data.slot.data.stepSequencer.buffer[pos] == 1))
                vel = 1;  // so we draw it
            if (weAreNotSolo)   // solo is on but we're not the current track, don't draw us 
                vel = 0; // don't draw if we're not solo
            uint8_t blink = (
                // draw play position cursor
                    ((local.stepSequencer.playState != PLAY_STATE_STOPPED) && 
                        ((d == local.stepSequencer.currentPlayPosition) ||   // main cursor
                        ((local.stepSequencer.currentEditPosition < 0 ) && (t == local.stepSequencer.currentTrack) && (abs(d - local.stepSequencer.currentPlayPosition) == 2)))) ||  // crosshatch
                // draw edit cursor
                ((t == local.stepSequencer.currentTrack) && (d == local.stepSequencer.currentEditPosition)) ||
                // draw mute or solo indicator          
                ((local.stepSequencer.muted[t] || weAreNotSolo) && (d == 0 || d == trackLen - 1)));
            if (vel || blink)
                {       
// <8 <16 <24 <32 <40 <48 <56 <64
				if (d < 32)				// only relevant for Mega
					{
					if (d < 16)
						{
						if (d < 8)
							{
                        	blinkOrSetPoint(led2, d, y, blink);
							}
						else // < 16
							{
                        	blinkOrSetPoint(led, d-8, y, blink);
							}
						}
					else
						{
						if (d < 24)
							{
                        	blinkOrSetPoint(led2, d-16, y-1, blink);
							}
						else  // < 32
							{
                        	blinkOrSetPoint(led, d-24, y-1, blink);
							}
						}
					}
				else
					{
					if (d < 48)
						{
						if (d < 40)
							{
                        	blinkOrSetPoint(led2, d - 32, y-2, blink);
							}
						else // < 48
							{
                        	blinkOrSetPoint(led, d-8 -32, y-2, blink);
							}
						}
					else
						{
						if (d < 56)
							{
                        	blinkOrSetPoint(led2, d-16 - 32, y-3, blink);
							}
						else  // < 64
							{
                        	blinkOrSetPoint(led, d-24 - 32, y-3, blink);
							}
						}
                    }
                }
            }
        y -= skip;
        }
        
    // Next draw the track number
    drawRange(led2, 0, 1, 12, local.stepSequencer.currentTrack);

    // Next the MIDI channel
    drawMIDIChannel(
        (local.stepSequencer.outMIDI[local.stepSequencer.currentTrack] == MIDI_OUT_DEFAULT) ?
        options.channelOut : local.stepSequencer.outMIDI[local.stepSequencer.currentTrack]);

#if defined(__AVR_ATmega2560__) 
    // Are we locked?
    if (data.slot.data.stepSequencer.locked)
        setPoint(led, 4, 1);
#endif

    // Do we have a fader value != FADER_IDENDITY_VALUE ?
    if (local.stepSequencer.fader[local.stepSequencer.currentTrack] != FADER_IDENTITY_VALUE)
        setPoint(led, 5, 1);
    
    // Are we overriding velocity?
    if (local.stepSequencer.velocity[local.stepSequencer.currentTrack] != STEP_SEQUENCER_NO_OVERRIDE_VELOCITY)
        setPoint(led, 6, 1);

    // Are we stopped?
    if (local.stepSequencer.playState != PLAY_STATE_PLAYING)
        setPoint(led, 7, 1);
    }





// Reformats the sequence as requested by the user
void stateStepSequencerFormat()
    {
    uint8_t result;
#if defined(__FOO__) 
    const char* menuItems[5] = {  PSTR("16 NOTES"), PSTR("24 NOTES"), PSTR("32 NOTES"), PSTR("48 NOTES"), PSTR("64 NOTES") };
    result = doMenuDisplay(menuItems, 5, STATE_NONE, 0, 1);
#else
    const char* menuItems[3] = {  PSTR("16 NOTES"), PSTR("24 NOTES"), PSTR("32 NOTES") };
    result = doMenuDisplay(menuItems, 3, STATE_NONE, 0, 1);
#endif
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            // we assume that all zeros is erased
            data.slot.type = SLOT_TYPE_STEP_SEQUENCER;
            data.slot.data.stepSequencer.format = currentDisplay;
            memset(data.slot.data.stepSequencer.buffer, 0, SLOT_DATA_SIZE - 2);
            for(uint8_t i = 0; i < GET_NUM_TRACKS(); i++)
                {
                resetTrack(i);
                }
            stopStepSequencer();
            goDownState(STATE_STEP_SEQUENCER_PLAY);
            }
        break;
        case MENU_CANCELLED:
            {
            goDownState(STATE_STEP_SEQUENCER);
            }
        break;
        }
    }


void removeSuccessiveTies(uint8_t p, uint8_t trackLen)
    {
    p = incrementAndWrap(p, trackLen);
        
    uint8_t v = (trackLen * local.stepSequencer.currentTrack + p) * 2 ;
    while((data.slot.data.stepSequencer.buffer[v + 1]== 0) &&
        data.slot.data.stepSequencer.buffer[v] == 1)
        {
        data.slot.data.stepSequencer.buffer[v] = 0;  // make it a rest
        p = incrementAndWrap(p, trackLen);
        }
                        
    // we gotta do this because we just deleted some notes :-(
    sendAllNotesOff();
    }
                                                

// Sends either a Note ON (if note is 0...127) or Note OFF (if note is 128...255)
// with the given velocity and the given track.  Computes the proper MIDI channel.
void sendTrackNote(uint8_t note, uint8_t velocity, uint8_t track)
    {
    uint8_t out = local.stepSequencer.outMIDI[track];
    if (out == MIDI_OUT_DEFAULT) 
        out = options.channelOut;
    if (out != NO_MIDI_OUT)
        if (note < 128) sendNoteOn(note, velocity, out);
        else sendNoteOff(note - 128, velocity, out);
    }



void loadBuffer(uint8_t position, uint8_t note, uint8_t velocity)
    {
    data.slot.data.stepSequencer.buffer[position * 2] = note;
    data.slot.data.stepSequencer.buffer[position * 2 + 1] = velocity;
    }


/*

int16_t getNewCursorXPos(uint8_t trackLen)
    {
    // pot / 2 * (tracklen + 1) / 9 - 1
    // we do it this way because pot * (tracklen + 1) / 10 - 1 exceeds 64K
    return ((int16_t)(((pot[RIGHT_POT] >> 1) * (trackLen + 1)) >> 9)) - 1;  ///  / 1024 - 1;
    }
*/



// I'd prefer the following code as it creates a bit of a buffer so scrolling to position 0 doesn't go straight
// into play position mode.  Or perhaps we should change things so that you scroll to the far RIGHT edge, dunno.
// But anyway we can't do this because adding just a little bit here radically increases our memory footprint. :-(
#define CURSOR_LEFT_MARGIN (4)

int16_t getNewCursorXPos(uint8_t trackLen)
	{
	int16_t val = ((int16_t)(((pot[RIGHT_POT] >> 1) * (trackLen + CURSOR_LEFT_MARGIN)) >> 9)) - CURSOR_LEFT_MARGIN;
	if ((val < 0) && (val > -CURSOR_LEFT_MARGIN))
		val = 0;
	return val;
	}




void resetStepSequencer()
    {
    local.stepSequencer.currentPlayPosition = GET_TRACK_LENGTH() - 1;
    }
        
void stopStepSequencer()
    {
    resetStepSequencer();
    local.stepSequencer.playState = PLAY_STATE_STOPPED;
    sendAllNotesOff();
    }


// Plays and records the sequence
void stateStepSequencerPlay()
    {
    // first we:
    // compute TRACKLEN, the length of the track
    // compute SKIP, the number of lines on the screen the track takes up
    uint8_t trackLen = GET_TRACK_LENGTH();
    uint8_t numTracks = GET_NUM_TRACKS();
    
//#if defined(__FOO__) 
	// this little function correctly maps:
	// 8 -> 1
	// 12 -> 1
	// 16 -> 1
	// 24 -> 2
	// 32 -> 2
	// 48 -> 3
	// 64 -> 4    
    uint8_t skip = ((trackLen + 15) >> 4);	// that is, trackLen / 16
//#else
//    uint8_t skip = (trackLen > 16 ? 2 : 1);
//#endif

    if (entry)
        {
        entry = false;
        local.stepSequencer.currentRightPot = -1;
        
/*
        // The vaguaries of the compiler tell me that if I remove this
        // code, the bytes go UP!!!!  A lot!  But I need to remove it, it's
        // wrong.  So I'm putting in dummy code which fools the compiler just enough.

        for(uint8_t i = 0; i < numTracks; i++)
            {
            //  (GET_TRACK_LENGTH() >> 8)   --->    0   but the compiler can't figure that out
            local.stepSequencer.noteOff[i] = local.stepSequencer.noteOff[i + (GET_TRACK_LENGTH() >> 8)];
            }
*/
        
        /*
          for(uint8_t i = 0; i < numTracks; i++)
          {
          local.stepSequencer.noteOff[i] = NO_NOTE;
          }
        */
        }

    if (isUpdated(BACK_BUTTON, RELEASED))
        {
        goUpState(STATE_STEP_SEQUENCER_SURE);
        }
    else if (isUpdated(MIDDLE_BUTTON, RELEASED))
        {
        if (local.stepSequencer.currentEditPosition >= 0)
            {
            // add a rest
            loadBuffer(trackLen * local.stepSequencer.currentTrack + local.stepSequencer.currentEditPosition, 0, 0);
            removeSuccessiveTies(local.stepSequencer.currentEditPosition, trackLen);
            local.stepSequencer.currentEditPosition = incrementAndWrap(local.stepSequencer.currentEditPosition, trackLen);  
            local.stepSequencer.currentRightPot = getNewCursorXPos(trackLen);
            }
        else    // toggle mute
            {
            local.stepSequencer.muted[local.stepSequencer.currentTrack] = !local.stepSequencer.muted[local.stepSequencer.currentTrack] ;
            }       
        }
    else if (isUpdated(MIDDLE_BUTTON, RELEASED_LONG))
        {
        if (local.stepSequencer.currentEditPosition >= 0)
            {
            // add a tie.
            // We only permit ties if (1) the note before was NOT a rest and
            // (2) the note AFTER is NOT another tie (to prevent us from making a full line of ties)
            // These two positions (before and after) are p and p2 
            uint8_t p = local.stepSequencer.currentEditPosition - 1;
            if (p == 255) p = trackLen - 1;		// we wrapped around from 0
            uint8_t p2 = p + 2;
            if (p2 >= trackLen) p2 = 0;
            
            uint8_t v = (trackLen * local.stepSequencer.currentTrack + p) * 2 ;
            uint8_t v2 = (trackLen * local.stepSequencer.currentTrack + p2) * 2 ;
            // don't add if a rest precedes it or a tie is after it
            if (((data.slot.data.stepSequencer.buffer[v + 1] == 0) &&           // rest before
                    (data.slot.data.stepSequencer.buffer[v] == 0)) ||
                    ((data.slot.data.stepSequencer.buffer[v2 + 1] == 1) &&          // tie after
                    (data.slot.data.stepSequencer.buffer[v2] == 0)))
                {
                // do nothing
                }
            else
                {
                loadBuffer(trackLen * local.stepSequencer.currentTrack + local.stepSequencer.currentEditPosition, 1, 0);
                local.stepSequencer.currentEditPosition = incrementAndWrap(local.stepSequencer.currentEditPosition, trackLen);
                local.stepSequencer.currentRightPot = getNewCursorXPos(trackLen);
                }
            }
        else 
#if defined(__AVR_ATmega2560__) 
        if (!data.slot.data.stepSequencer.locked)
#endif
            {
            // do a "light" clear, not a full reset
            memset(data.slot.data.stepSequencer.buffer + trackLen * local.stepSequencer.currentTrack * 2, 0, trackLen * 2);
            }
        }
    else if (isUpdated(SELECT_BUTTON, RELEASED))
        {
#if defined(__AVR_ATmega2560__)
        if (options.stepSequencerSendClock)
        	{
        	// we always stop the clock just in case, even if we're immediately restarting it
        	stopClock(true);
        	}
#endif
        switch(local.stepSequencer.playState)
            {
            case PLAY_STATE_STOPPED:
                {
                local.stepSequencer.playState = PLAY_STATE_WAITING;
#if defined(__AVR_ATmega2560__)
				if (options.stepSequencerSendClock)
					{
					// Possible bug condition:
					// The MIDI spec says that there "should" be at least 1 ms between
					// starting the clock and the first clock pulse.  I don't know if that
					// will happen here consistently.
					startClock(true);
					}
#endif                
                }
            break;
            case PLAY_STATE_WAITING:
                // Fall Thru
            case PLAY_STATE_PLAYING:
                {
                stopStepSequencer();
                }
            break;
            }
        }
    else if (isUpdated(SELECT_BUTTON, RELEASED_LONG))
        {
        state = STATE_STEP_SEQUENCER_MENU;
        entry = true;
        }
    else if (potUpdated[LEFT_POT])
        {
        local.stepSequencer.currentTrack = ((pot[LEFT_POT] * numTracks) >> 10);         //  / 1024;
        local.stepSequencer.currentTrack = bound(local.stepSequencer.currentTrack, 0, numTracks);
        }
    else if (potUpdated[RIGHT_POT])
        {
        int16_t newPos = getNewCursorXPos(trackLen);
        if (lockoutPots ||      // using an external NRPN device, which is likely accurate
            local.stepSequencer.currentRightPot == -1 ||   // nobody's been entering data
            local.stepSequencer.currentRightPot >= newPos && local.stepSequencer.currentRightPot - newPos >= 2 ||
            local.stepSequencer.currentRightPot < newPos && newPos - local.stepSequencer.currentRightPot >= 2)
            {
            local.stepSequencer.currentEditPosition = newPos;
                        
            if (local.stepSequencer.currentEditPosition >= trackLen)        
                local.stepSequencer.currentEditPosition = trackLen - 1;
            local.stepSequencer.currentRightPot = -1;
            }
        }
    else if (newItem && (itemType == MIDI_NOTE_ON) //// there is a note played
#if defined(__AVR_ATmega2560__) 
			&& !data.slot.data.stepSequencer.locked  
#endif
	)
        {
        TOGGLE_IN_LED();
        uint8_t note = itemNumber;
        uint8_t velocity = itemValue;
                
        // here we're trying to provide some slop so the user can press the note early.
        // we basically are rounding up or down to the nearest note
        uint8_t pos = local.stepSequencer.currentEditPosition >= 0 ? 
            local.stepSequencer.currentEditPosition :
            local.stepSequencer.currentPlayPosition + (notePulseCountdown <= (notePulseRate >> 1) ? 1 : 0);
        if (pos >= trackLen) pos = 0;
        if (pos < 0) pos = trackLen - 1;

        // add a note
        loadBuffer(trackLen * local.stepSequencer.currentTrack + pos, note, velocity);
        removeSuccessiveTies(pos, trackLen);

        if (local.stepSequencer.currentEditPosition >= 0)
            {
            local.stepSequencer.currentEditPosition = incrementAndWrap(local.stepSequencer.currentEditPosition, trackLen);
            }
        else 
        	{
#if defined(__AVR_ATmega2560__)        
        	local.stepSequencer.dontPlay[local.stepSequencer.currentTrack] = 1;
        	if (!options.stepSequencerNoEcho)          // only play if we're echoing
        	    {
        	    sendTrackNote(note, velocity, local.stepSequencer.currentTrack);
        	    }
#endif
			}
			
        local.stepSequencer.currentRightPot = getNewCursorXPos(trackLen);
        }
    else if (newItem && (itemType == MIDI_NOTE_OFF)
#if defined(__AVR_ATmega2560__) 
			&& !data.slot.data.stepSequencer.locked  
#endif
    )
    	{
    	sendTrackNote(itemNumber + 128, itemValue, local.stepSequencer.currentTrack);
    	}
        
    playStepSequencer();
    if (updateDisplay)
        {
        drawStepSequencer(trackLen, numTracks, skip);
        }
    }


// Various choices in the menu
#define STEP_SEQUENCER_MENU_SOLO 0
#define STEP_SEQUENCER_MENU_LENGTH 1
#define STEP_SEQUENCER_MENU_MIDI_OUT 2
#define STEP_SEQUENCER_MENU_VELOCITY 3
#define STEP_SEQUENCER_MENU_FADER 4

#if defined(__AVR_ATmega2560__)
#define STEP_SEQUENCER_MENU_RESET 5
#define STEP_SEQUENCER_MENU_SAVE 6
#define STEP_SEQUENCER_MENU_SEND_CLOCK 7
#define STEP_SEQUENCER_MENU_LOCK 8
#define STEP_SEQUENCER_MENU_NO_ECHO 9
#define STEP_SEQUENCER_MENU_OPTIONS 10
#else
#define STEP_SEQUENCER_MENU_RESET 5
#define STEP_SEQUENCER_MENU_SAVE 6
#define STEP_SEQUENCER_MENU_LOCK 7
#define STEP_SEQUENCER_MENU_OPTIONS 8
#endif



// Gives other options
void stateStepSequencerMenu()
    {
    uint8_t result;

#if defined(__AVR_ATmega2560__)    
    const char* menuItems[11] = {    
        (local.stepSequencer.solo) ? PSTR("NO SOLO") : PSTR("SOLO"),
        PSTR("LENGTH (TRACK)"),
        PSTR("OUT MIDI (TRACK)"),
        PSTR("VELOCITY (TRACK)"),
        PSTR("FADER (TRACK)"), 
        PSTR("RESET TRACK"),
        PSTR("SAVE"), 
        options.stepSequencerSendClock ? PSTR("NO CLOCK CONTROL") : PSTR("CLOCK CONTROL"),
        data.slot.data.stepSequencer.locked ? PSTR("UNLOCK") : PSTR("LOCK"),
        options.stepSequencerNoEcho ? PSTR("ECHO") : PSTR("NO ECHO"), 
        options_p 
        };
    result = doMenuDisplay(menuItems, 11, STATE_NONE, STATE_NONE, 1);
#else
    const char* menuItems[8] = {    
        (local.stepSequencer.solo) ? PSTR("NO SOLO") : PSTR("SOLO"),
        PSTR("LENGTH (TRACK)"),
        PSTR("OUT MIDI (TRACK)"),
        PSTR("VELOCITY (TRACK)"),
        PSTR("FADER (TRACK)"), 
        PSTR("RESET TRACK"),
        PSTR("SAVE"), 
        options_p 
        };
    result = doMenuDisplay(menuItems, 8, STATE_NONE, STATE_NONE, 1);
#endif

    playStepSequencer();
    switch (result)
        {
        case NO_MENU_SELECTED:
            {
            // do nothing
            }
        break;
        case MENU_SELECTED:
            {
            state = STATE_STEP_SEQUENCER_PLAY;
            entry = true;
            switch(currentDisplay)
                {
                case STEP_SEQUENCER_MENU_SOLO:
                    {
                    local.stepSequencer.solo = !local.stepSequencer.solo;
                    }
                break;
                case STEP_SEQUENCER_MENU_LENGTH:
                    {
                    state = STATE_STEP_SEQUENCER_LENGTH;                            
                    }
                break;
                case STEP_SEQUENCER_MENU_MIDI_OUT:
                    {
                    local.stepSequencer.backup = local.stepSequencer.outMIDI[local.stepSequencer.currentTrack];
                    state = STATE_STEP_SEQUENCER_MIDI_CHANNEL_OUT;
                    }
                break;
                case STEP_SEQUENCER_MENU_VELOCITY:
                    {
                    state = STATE_STEP_SEQUENCER_VELOCITY;
                    }
                break;
                case STEP_SEQUENCER_MENU_FADER:
                    {
                    state = STATE_STEP_SEQUENCER_FADER;
                    }
                break;
                case STEP_SEQUENCER_MENU_RESET:
                    {
                    uint8_t trackLen = GET_TRACK_LENGTH();
                    resetTrack(local.stepSequencer.currentTrack);
                    break;
                    }
                case STEP_SEQUENCER_MENU_SAVE:
                    {
                    state = STATE_STEP_SEQUENCER_SAVE;
                    }
                break;
#if defined(__AVR_ATmega2560__)    
                case STEP_SEQUENCER_MENU_SEND_CLOCK:
                    {
                    options.stepSequencerSendClock = !options.stepSequencerSendClock;
                    saveOptions();
                    }
                break;
                case STEP_SEQUENCER_MENU_LOCK:
                    {
                    data.slot.data.stepSequencer.locked = !data.slot.data.stepSequencer.locked;
                    }
                break;
                case STEP_SEQUENCER_MENU_NO_ECHO:
                    {
                    options.stepSequencerNoEcho = !options.stepSequencerNoEcho;
                    saveOptions();
                    }
                break;
#endif
                case STEP_SEQUENCER_MENU_OPTIONS:
                    {
                    optionsReturnState = STATE_STEP_SEQUENCER_MENU;
                    goDownState(STATE_OPTIONS);
                    }
                break;
                }
            }
        break;
        case MENU_CANCELLED:
            {
            // get rid of any residual select button calls, so we don't stop when exiting here
            isUpdated(SELECT_BUTTON, RELEASED);
            goUpState(STATE_STEP_SEQUENCER_PLAY);
            }
        break;
        }
        
    }

// Turns off all notes
void clearNotesOnTracks(uint8_t clearEvenIfNoteNotFinished)
    {
    uint8_t numTracks = GET_NUM_TRACKS();
    uint8_t trackLen = GET_TRACK_LENGTH();
    for(uint8_t track = 0; track < numTracks; track++)
        {
        // clearNotesOnTracks is called BEFORE the current play position is incremented.
        // So we need to check the NEXT note to determine if it's a tie.  If it is,
        // then we don't want to stop playing

        uint8_t currentPos = local.stepSequencer.currentPlayPosition + 1;       // next position
        if (currentPos >= trackLen) currentPos = 0;                                                     // wrap around
        uint16_t pos = (track * (uint16_t) trackLen + currentPos) * 2;
        uint8_t vel = data.slot.data.stepSequencer.buffer[pos + 1];
        uint8_t note = data.slot.data.stepSequencer.buffer[pos];
        
        if (local.stepSequencer.noteOff[track] < NO_NOTE &&             // there's something to turn off
            (clearEvenIfNoteNotFinished || (currentTime >= local.stepSequencer.offTime[track])) &&  // it's time to clear the note
            (!((vel == 0) && (note == 1))))                                     // not a tie
            {
            uint8_t out = (local.stepSequencer.outMIDI[track] == MIDI_OUT_DEFAULT ? options.channelOut : local.stepSequencer.outMIDI[track]);
            if (out != NO_MIDI_OUT)
                {
                sendNoteOff(local.stepSequencer.noteOff[track], 127, out);
                }
            local.stepSequencer.noteOff[track] = NO_NOTE;
            }
        }
    }


// Plays the current sequence
void playStepSequencer()
    {
    // we redo this rather than take it from stateStepSequencerPlay because we may be 
    // called from other methods as well 
        
    uint8_t trackLen = GET_TRACK_LENGTH();
    uint8_t numTracks = GET_NUM_TRACKS();
        
    // Clear notes if necessary
    clearNotesOnTracks(false);
        
    if ((local.stepSequencer.playState == PLAY_STATE_WAITING) && beat)
        local.stepSequencer.playState = PLAY_STATE_PLAYING;
        
    if (notePulse && (local.stepSequencer.playState == PLAY_STATE_PLAYING))
        {
        // definitely clear everything
        clearNotesOnTracks(true);

        local.stepSequencer.currentPlayPosition = incrementAndWrap(local.stepSequencer.currentPlayPosition, trackLen);

        for(uint8_t track = 0; track < numTracks; track++)
            {
            // data is stored per-track as
            // NOTE VEL
            // We need to strip off the high bit because it's used for other packing
                                
            // for each note in the track
            uint16_t pos = (track * (uint16_t) trackLen + local.stepSequencer.currentPlayPosition) * 2;
            uint8_t vel = data.slot.data.stepSequencer.buffer[pos + 1];
            uint8_t note = data.slot.data.stepSequencer.buffer[pos];
            uint8_t noteLength = ((local.stepSequencer.noteLength[track] == PLAY_LENGTH_USE_DEFAULT) ? 
                options.noteLength : local.stepSequencer.noteLength[track] );
                        
            if (vel == 0 && note == 1)  // tie
                {
                local.stepSequencer.offTime[track] = currentTime + (div100(notePulseRate * getMicrosecsPerPulse() * noteLength));
                }
            else if (vel == 0 && note == 0) // rest
                {
                local.stepSequencer.noteOff[track] = NO_NOTE;
                }
            else if (vel != 0 
#if defined(__AVR_ATmega2560__)        
            && !local.stepSequencer.dontPlay[track]  // not a rest or tie
#endif
				)            
                {
                if (local.stepSequencer.velocity[track] != STEP_SEQUENCER_NO_OVERRIDE_VELOCITY)
                    vel = local.stepSequencer.velocity[track];
                    
                if ((!local.stepSequencer.muted[track]) &&                      // if we're not muted
                    ((!(local.stepSequencer.solo) || track == local.stepSequencer.currentTrack)))  // solo is turned off, or solo is turne don AND we're the current track
                    {
                    uint16_t newvel = (vel * (uint16_t)(local.stepSequencer.fader[track])) >> 5;
                    if (newvel > 127) 
                        newvel = 127;
                    sendTrackNote(note, (uint8_t)newvel, track);         // >> 5 is / FADER_IDENTITY_VALUE, that is, / 32
                    }
                local.stepSequencer.offTime[track] = currentTime + (div100(notePulseRate * getMicrosecsPerPulse() * noteLength));
                local.stepSequencer.noteOff[track] = note;
                }
            }
#if defined(__AVR_ATmega2560__)                    
        // clear the dontPlay flags
        memset(local.stepSequencer.dontPlay, 0, numTracks);
#endif
        }

    // click track
    doClick();
    }
        
        

