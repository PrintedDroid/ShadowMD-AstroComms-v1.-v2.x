/*
 * =============================================================================
 * DFPlayer Mini Sound Controller for Betterduino/Benduino/Marcduino
 * Version: 1.2 - 2026/04
 * www.printeddroid.com
 * =============================================================================
 *
 * CHANGES SINCE V1.0:
 * - DFPlayer command changed from 0x12 (Play Track by global index) to 0x14
 *   (Play from MP3 folder by 4-digit filename prefix).
 *   Same SD card layout as MarcDuino V3 DFPlayer-Mode and BetterDuino
 *   DFPlayer-Mode — one common "mp3/0001_*.mp3 ... 0255_*.mp3" layout.
 * - Only the leading 4 digits of each filename are used by the DFPlayer.
 *   The part after the underscore (e.g. "_leia-1") is a human-readable
 *   description and is ignored by the hardware — pick anything that helps
 *   you remember what that sound is.
 *
 * DESCRIPTION:
 * This sketch allows you to control a DFPlayer Mini MP3 module using serial
 * commands. It's designed to be compatible with Astromech control systems (ShadowMD based)
 * but can be used for any project requiring sound playback control.
 *
 * HARDWARE REQUIRED:
 * - Arduino (Uno, Nano, Mega, etc.)
 * - DFPlayer Mini MP3 module
 * - MicroSD card (up to 32GB, formatted as FAT32)
 * - Speaker (3W to 8W recommended) / AMP
 * - 1K ohm resistor (important for TX line protection)
 *
 * WIRING CONNECTIONS:
 * DFPlayer Mini -> Arduino
 * - VCC ---------> 5V
 * - GND ---------> GND
 * - RX ----------> Pin 11 (through 1K resistor - IMPORTANT!)
 * - TX ----------> Pin 10 (direct connection)
 * - SPK_1 -------> Speaker positive
 * - SPK_2 -------> Speaker negative
 *
 * SD CARD PREPARATION (MarcDuino-V3-compatible):
 * 1. Format your SD card as FAT32.
 * 2. Create a folder called "mp3" in the SD card root.
 * 3. Place numbered MP3 files inside the mp3 folder:
 *    0001_anyname.mp3, 0002_anyname.mp3, ... 0255_anyname.mp3
 *    The first 4 digits MUST be the number (with leading zeros). Anything
 *    after the number is ignored by the DFPlayer — use it for your own
 *    description (e.g. "0151_leia-message.mp3").
 *
 * SOUND ORGANIZATION:
 * Files are organized in 9 "banks" of 25 sounds each (MarcDuino convention):
 * - Bank 1: Files 0001-0025 (General sounds)
 * - Bank 2: Files 0026-0050 (Chat/beep sounds)
 * - Bank 3: Files 0051-0075 (Happy sounds)
 * - Bank 4: Files 0076-0100 (Sad sounds)
 * - Bank 5: Files 0101-0125 (Whistle sounds)
 * - Bank 6: Files 0126-0150 (Scream sounds)
 * - Bank 7: Files 0151-0175 (Leia/message sounds)
 * - Bank 8: Files 0176-0200 (Music / Sing / Cantina sounds)
 * - Bank 9: Files 0201-0225 (reserved / extras)
 * 
 * COMMAND FORMAT:
 * Send commands via serial monitor at 9600 baud
 * All commands start with $ followed by parameters
 * End each command with Enter (carriage return)
 * 
 * BASIC COMMANDS:
 * $xyy - Play sound (x=bank number, yy=sound number)
 *        Examples: $101 plays bank 1, sound 1
 *                  $225 plays bank 2, sound 25
 *                  $1   plays next sound in bank 1
 * 
 * SPECIAL COMMANDS:
 * $R - Start random sound mode (plays random sounds every 6-10 seconds)
 * $O - Sound off (mutes volume)
 * $s - Stop current playback
 * $+ - Volume up
 * $- - Volume down
 * $m - Set volume to middle
 * $f - Set volume to maximum
 * $p - Set volume to minimum
 * $L - Play Leia message (bank 7, sound 1)
 * $C - Play Cantina music (bank 9, sound 5)
 * $c - Play beep cantina (bank 9, sound 1)
 * $S - Play scream (bank 6, sound 1)
 * $F - Play faint/short circuit (bank 6, sound 3)
 * $D - Play disco (bank 9, sound 6)
 * $W - Play Star Wars theme (bank 9, sound 2)
 * $M - Play Imperial March (bank 9, sound 3)
 * 
 * TROUBLESHOOTING:
 * - No sound: Check speaker connections and volume
 * - Distorted sound: Use 1K resistor on TX line, check power supply
 * - Commands not working: Verify 9600 baud rate, check wiring
 * - Files not playing: Ensure correct file naming (001-xxx.mp3 format)
 * 
 * =============================================================================
 */

#include <SoftwareSerial.h>

// =============================================================================
// PIN CONFIGURATION - Change these if using different pins
// =============================================================================
#define DFPLAYER_RX_PIN 10  // Arduino pin connected to DFPlayer TX
#define DFPLAYER_TX_PIN 11  // Arduino pin connected to DFPlayer RX (use 1K resistor!)

// =============================================================================
// VOLUME SETTINGS - Adjust these to your preference
// =============================================================================
#define DF_VOLUME_MIN   1   // Minimum volume (1-30)
#define DF_VOLUME_MAX   30  // Maximum volume (1-30)
#define DF_VOLUME_MID   15  // Default/middle volume
#define DF_VOLUME_OFF   0   // Mute
#define DF_VOLUME_STEPS 15  // Number of volume steps for +/- commands

// =============================================================================
// SOUND BANK CONFIGURATION - Modify if you have different numbers of sounds
// =============================================================================
#define MAX_BANKS           9   // Total number of sound banks
#define MAX_SOUNDS_PER_BANK 25  // Maximum sounds in each bank
#define BANK_CUTOFF        4    // Banks 1-4 cycle through sounds, 5-9 play first

// This array defines how many actual sound files you have in each bank
// Adjust these numbers to match your actual MP3 files
const uint8_t bank_max_sounds[] = {
  19,  // Bank 1: How many files from 001-025 you actually have
  18,  // Bank 2: How many files from 026-050 you actually have
  7,   // Bank 3: How many files from 051-075 you actually have
  4,   // Bank 4: How many files from 076-100 you actually have
  3,   // Bank 5: How many files from 101-125 you actually have
  25,  // Bank 6: How many files from 126-150 you actually have
  25,  // Bank 7: How many files from 151-175 you actually have
  25,  // Bank 8: How many files from 176-200 you actually have
  25   // Bank 9: How many files from 201-225 you actually have
};

// =============================================================================
// GLOBAL VARIABLES - Used internally by the program
// =============================================================================
SoftwareSerial dfSerial(DFPLAYER_RX_PIN, DFPLAYER_TX_PIN); // Communication with DFPlayer
uint8_t current_volume = DF_VOLUME_MID;     // Current volume level
uint8_t bank_indexes[MAX_BANKS] = {0};      // Tracks current position in each bank
unsigned long random_timer = 0;             // Timer for random sound intervals
unsigned long random_interval = 0;          // Time until next random sound
bool random_mode = false;                   // Is random mode active?
bool random_suspended = false;              // Is random mode temporarily paused?

// =============================================================================
// SETUP FUNCTION - Runs once when Arduino starts
// =============================================================================
void setup() {
  // Initialize serial communication with computer (for commands)
  Serial.begin(9600);
  Serial.println(F("========================================"));
  Serial.println(F("DFPlayer Mini Controller Starting..."));
  Serial.println(F("========================================"));
  
  // Initialize communication with DFPlayer
  dfSerial.begin(9600);
  
  // Wait for DFPlayer to fully initialize (important!)
  Serial.println(F("Waiting for DFPlayer to initialize..."));
  delay(3000);
  
  // Configure DFPlayer settings
  setEqualizer(0);          // Set normal equalizer (0=Normal, 1=Pop, 2=Rock, 3=Jazz, 4=Classic, 5=Bass)
  setVolume(current_volume); // Set initial volume
  
  // Print ready message with instructions
  Serial.println(F(""));
  Serial.println(F("DFPlayer Ready! Send commands:"));
  Serial.println(F("  $101 = Play bank 1, sound 1"));
  Serial.println(F("  $R   = Random mode"));
  Serial.println(F("  $+   = Volume up"));
  Serial.println(F("  $-   = Volume down"));
  Serial.println(F("  $s   = Stop playback"));
  Serial.println(F("========================================"));
}

// =============================================================================
// MAIN LOOP - Runs continuously
// =============================================================================
void loop() {
  // ---------------------------------------------
  // PART 1: Check for incoming serial commands
  // ---------------------------------------------
  if (Serial.available()) {
    static String command = "";  // Buffer to build command string
    char ch = Serial.read();     // Read one character
    
    // Check if command is complete (user pressed Enter)
    if (ch == '\r' || ch == '\n') {
      if (command.length() > 0) {
        Serial.print(F("Received command: "));
        Serial.println(command);
        processCommand(command);  // Process the complete command
        command = "";             // Clear buffer for next command
      }
    } else {
      command += ch;  // Add character to command buffer
    }
  }
  
  // ---------------------------------------------
  // PART 2: Handle automatic random sound playback
  // ---------------------------------------------
  if (random_mode && !random_suspended) {
    // Check if it's time to play next random sound
    if (millis() - random_timer > random_interval) {
      playRandomSound();
      random_timer = millis();  // Reset timer
      // Set random interval between 6 and 10 seconds
      random_interval = random(6000, 10000);
    }
  }
}

// =============================================================================
// COMMAND PROCESSING - Interprets and executes commands
// =============================================================================
void processCommand(String cmd) {
  // Validate command format (must start with $ and have at least 2 characters)
  if (cmd.length() < 2 || cmd[0] != '$') {
    Serial.println(F("ERROR: Invalid command format"));
    Serial.println(F("Commands must start with $ (e.g., $101 or $R)"));
    return;
  }
  
  // Get the command character (second character after $)
  char cmdChar = cmd[1];
  
  // ---------------------------------------------
  // Check if it's a NUMERIC command (play specific sound)
  // ---------------------------------------------
  if (isDigit(cmdChar)) {
    stopRandom();  // Stop random mode when playing specific sound
    
    uint8_t bank = cmdChar - '0';  // Convert character to number
    uint8_t sound = 0;              // Default to 0 (play next in sequence)
    
    // Check if sound number was specified
    if (cmd.length() > 2) {
      sound = cmd.substring(2).toInt();  // Get sound number
    }
    
    playSound(bank, sound);  // Play the requested sound
    return;
  }
  
  // ---------------------------------------------
  // Process CHARACTER commands (special functions)
  // ---------------------------------------------
  switch(cmdChar) {
    case 'R':  // Random mode - plays random sounds automatically
      startRandom();
      break;
      
    case 'O':  // Off - mutes audio
      stopRandom();
      setVolume(DF_VOLUME_OFF);
      Serial.println(F("Audio muted"));
      break;
      
    case 's':  // Stop - stops current playback
      stopRandom();
      stopPlayback();
      Serial.println(F("Playback stopped"));
      break;
      
    case '+':  // Volume up
      volumeUp();
      Serial.print(F("Volume: "));
      Serial.println(current_volume);
      break;
      
    case '-':  // Volume down
      volumeDown();
      Serial.print(F("Volume: "));
      Serial.println(current_volume);
      break;
      
    case 'm':  // Middle volume
      current_volume = DF_VOLUME_MID;
      setVolume(current_volume);
      Serial.println(F("Volume set to middle"));
      break;
      
    case 'f':  // Full/maximum volume
      current_volume = DF_VOLUME_MAX;
      setVolume(current_volume);
      Serial.println(F("Volume set to maximum"));
      break;
      
    case 'p':  // Minimum volume (quiet)
      current_volume = DF_VOLUME_MIN;
      setVolume(current_volume);
      Serial.println(F("Volume set to minimum"));
      break;
      
    // Special sound shortcuts
    case 'L':  // Leia message
      Serial.println(F("Playing: Leia message"));
      suspendRandom();
      playSound(7, 1);
      resumeRandomDelayed(44000);  // Resume random after 44 seconds
      break;
      
    case 'C':  // Cantina music (full orchestral)
      Serial.println(F("Playing: Cantina music"));
      suspendRandom();
      playSound(9, 5);
      resumeRandomDelayed(56000);  // Resume random after 56 seconds
      break;
      
    case 'c':  // Cantina beeps (R2-D2 version)
      Serial.println(F("Playing: Beep cantina"));
      suspendRandom();
      playSound(9, 1);
      resumeRandomDelayed(27000);  // Resume random after 27 seconds
      break;
      
    case 'S':  // Scream sound
      Serial.println(F("Playing: Scream"));
      suspendRandom();
      playSound(6, 1);
      resumeRandomDelayed(5000);   // Resume random after 5 seconds
      break;
      
    case 'F':  // Faint/short circuit sound
      Serial.println(F("Playing: Faint/Short circuit"));
      suspendRandom();
      playSound(6, 3);
      resumeRandomDelayed(5000);   // Resume random after 5 seconds
      break;
      
    case 'D':  // Disco music
      Serial.println(F("Playing: Disco"));
      suspendRandom();
      playSound(9, 6);
      resumeRandomDelayed(396000); // Resume random after 6:36
      break;
      
    case 'W':  // Star Wars theme
      Serial.println(F("Playing: Star Wars theme"));
      stopRandom();  // Don't resume random after this
      playSound(9, 2);
      break;
      
    case 'M':  // Imperial March
      Serial.println(F("Playing: Imperial March"));
      stopRandom();  // Don't resume random after this
      playSound(9, 3);
      break;
      
    default:
      Serial.print(F("ERROR: Unknown command character: "));
      Serial.println(cmdChar);
      break;
  }
}

// =============================================================================
// SOUND PLAYBACK FUNCTIONS
// =============================================================================

/**
 * Play a specific sound from a bank
 * @param bank: Bank number (0-9, where 0 plays exact file number)
 * @param sound: Sound number within bank (0 plays next in sequence)
 */
void playSound(uint8_t bank, uint8_t sound) {
  uint16_t fileNum;  // Actual file number to play (001-255)
  
  // Validate bank number
  if (bank > MAX_BANKS) {
    Serial.println(F("ERROR: Invalid bank number"));
    return;
  }
  
  // Bank 0 is special - plays exact file number
  if (bank == 0) {
    fileNum = sound;
  }
  // Banks 1-9: Calculate file number based on bank and sound
  else if (sound != 0) {
    // Play specific sound number
    fileNum = (bank - 1) * MAX_SOUNDS_PER_BANK + sound;
    
    // Update the bank's current position for "next" functionality
    if (sound <= bank_max_sounds[bank - 1]) {
      bank_indexes[bank - 1] = sound;
    } else {
      bank_indexes[bank - 1] = bank_max_sounds[bank - 1];
    }
  }
  // Sound 0 means "play next" or "play first" depending on bank
  else {
    if (bank <= BANK_CUTOFF) {
      // Banks 1-4: Play next sound in sequence
      if (++bank_indexes[bank - 1] > bank_max_sounds[bank - 1]) {
        bank_indexes[bank - 1] = 1;  // Loop back to first
      }
      sound = bank_indexes[bank - 1];
    } else {
      // Banks 5-9: Always play first sound
      sound = 1;
    }
    fileNum = (bank - 1) * MAX_SOUNDS_PER_BANK + sound;
  }
  
  // Send play command to DFPlayer
  playDFP(fileNum);
  
  // Print confirmation
  Serial.print(F("Playing file "));
  Serial.print(fileNum);
  Serial.print(F(" (Bank "));
  Serial.print(bank);
  Serial.print(F(", Sound "));
  Serial.print(sound);
  Serial.println(F(")"));
}

/**
 * Play a random sound from banks 1-5
 */
void playRandomSound() {
  // Calculate total number of sounds in banks 1-5
  uint16_t totalSounds = 0;
  for (uint8_t i = 0; i < 5; i++) {
    totalSounds += bank_max_sounds[i];
  }
  
  // Pick a random number
  uint16_t randomNum = random(1, totalSounds + 1);
  uint16_t cumulative = 0;
  
  // Find which bank this random number falls into
  for (uint8_t bank = 1; bank <= 5; bank++) {
    cumulative += bank_max_sounds[bank - 1];
    if (randomNum <= cumulative) {
      uint8_t sound = randomNum - (cumulative - bank_max_sounds[bank - 1]);
      Serial.print(F("Random: "));
      playSound(bank, sound);
      return;
    }
  }
}

// =============================================================================
// RANDOM MODE CONTROL FUNCTIONS
// =============================================================================

/**
 * Start automatic random sound playback
 */
void startRandom() {
  random_mode = true;
  random_suspended = false;
  random_timer = millis();
  random_interval = random(6000, 10000);  // 6-10 seconds between sounds
  Serial.println(F("Random mode: ON"));
  Serial.println(F("(Playing random sounds every 6-10 seconds)"));
}

/**
 * Stop automatic random sound playback
 */
void stopRandom() {
  random_mode = false;
  random_suspended = false;
  Serial.println(F("Random mode: OFF"));
}

/**
 * Temporarily pause random mode (used during special sounds)
 */
void suspendRandom() {
  random_suspended = true;
}

/**
 * Resume random mode after a delay
 * @param delay_ms: Milliseconds to wait before resuming
 */
void resumeRandomDelayed(unsigned long delay_ms) {
  random_timer = millis() + delay_ms;
  random_suspended = false;
}

// =============================================================================
// VOLUME CONTROL FUNCTIONS
// =============================================================================

/**
 * Increase volume by one step
 */
void volumeUp() {
  uint8_t step = DF_VOLUME_MAX / DF_VOLUME_STEPS;
  
  if (current_volume >= DF_VOLUME_MAX) {
    current_volume = DF_VOLUME_MAX;
  } else if (current_volume + step > DF_VOLUME_MAX) {
    current_volume = DF_VOLUME_MAX;
  } else {
    current_volume += step;
  }
  
  setVolume(current_volume);
}

/**
 * Decrease volume by one step
 */
void volumeDown() {
  uint8_t step = DF_VOLUME_MAX / DF_VOLUME_STEPS;
  
  if (current_volume <= DF_VOLUME_MIN) {
    current_volume = DF_VOLUME_MIN;
  } else if (current_volume - step < DF_VOLUME_MIN) {
    current_volume = DF_VOLUME_MIN;
  } else {
    current_volume -= step;
  }
  
  setVolume(current_volume);
}

// =============================================================================
// DFPLAYER COMMUNICATION PROTOCOL
// =============================================================================

/**
 * Send a command to the DFPlayer
 * Commands use a specific protocol with checksum
 * @param cmd: 10-byte command array
 */
void sendDFP(uint8_t *cmd) {
  // Calculate checksum (bytes 2-7)
  uint16_t checksum = 0;
  for (int i = 2; i < 8; i++) {
    checksum += cmd[i];
  }
  
  // Add checksum to command (2's complement)
  cmd[7] = (uint8_t)~(checksum >> 8);
  cmd[8] = (uint8_t)~(checksum);
  
  // Send all 10 bytes to DFPlayer
  for (int i = 0; i < 10; i++) {
    dfSerial.write(cmd[i]);
  }
  
  // Small delay for DFPlayer to process command
  delay(10);
}

/**
 * Play a specific file number from the "mp3" folder
 * @param fileNum: File number (1-9999). The DFPlayer looks for
 *                 "mp3/NNNN_*.mp3" where NNNN is zero-padded fileNum.
 *
 * Command 0x14 = "Play from MP3 folder" — selects the track by the
 * 4-digit number prefix. Anything after the number in the filename is
 * ignored. This is the MarcDuino-V3 / BetterDuino standard layout.
 */
void playDFP(uint16_t fileNum) {
  // Command structure: Start, Version, Length, Command, Feedback, Para1, Para2, Checksum1, Checksum2, End
  uint8_t cmd[10] = {0x7E, 0xFF, 0x06, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF};
  cmd[5] = (uint8_t)(fileNum >> 8);  // High byte of file number
  cmd[6] = (uint8_t)(fileNum);       // Low byte of file number
  sendDFP(cmd);
}

/**
 * Stop current playback
 */
void stopPlayback() {
  uint8_t cmd[10] = {0x7E, 0xFF, 0x06, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF};
  sendDFP(cmd);
}

/**
 * Set playback volume
 * @param vol: Volume level (0-30)
 */
void setVolume(uint8_t vol) {
  uint8_t cmd[10] = {0x7E, 0xFF, 0x06, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF};
  cmd[6] = vol;
  sendDFP(cmd);
}

/**
 * Set equalizer mode
 * @param eq: Equalizer setting (0=Normal, 1=Pop, 2=Rock, 3=Jazz, 4=Classic, 5=Bass)
 */
void setEqualizer(uint8_t eq) {
  uint8_t cmd[10] = {0x7E, 0xFF, 0x06, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF};
  cmd[6] = eq;
  sendDFP(cmd);
}