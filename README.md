# Guitar Hero Game

## Introduction
This Rhythm Game has three to four buttons that need to be pressed when notes reach the start bar. The concept of this game is to match the notes with a corresponding button press at certain times and areas in the play area. If the player presses the button at the correct time then he/she would get points. The player is only to be pressing one at a time or else points are not granted. The point of the game is for the player to gain as much points as possible. Once the song is completed, the player would see their score and if their score is higher than the previous high score then it would replace it in the high score screen, otherwise it would stay the same.

<img src="https://github.com/codyiskhuu/120b_Final_Project/blob/master/Interface_2.PNG" width="500" height="250"> 

## Hardware
### Parts List
The hardware that was used in this design is listed below. 

ATMega1284p microcontroller
Buttons
Speaker
8x8 LED Matrix
4 Pin LCD Screen (instead of 8 from the lab)

## Pinout
<img src="https://github.com/codyiskhuu/120b_Final_Project/blob/master/Wiring.jpg" width="250" height="300">

## Software

<img src="https://github.com/codyiskhuu/120b_Final_Project/blob/master/Wiring.jpg" width="600" height="400">

<img src="https://github.com/codyiskhuu/120b_Final_Project/blob/master/FSM%20LCD_Display%20and%20Game_Logic.png" width="300" height="200">
<img src="https://github.com/codyiskhuu/120b_Final_Project/blob/master/FSM%20LCD_Display.png" width="150" height="150">
<img src="https://github.com/codyiskhuu/120b_Final_Project/blob/master/FSM%20Song_State.png" width="150" height="150">

## Complexities

### Completed Complexities:
*8x8 LED Matrix: Shows when you need to press inputs at the correct time 
*EEPROM: To save the High Score
*4 Pin LCD Display: To save space
*Core Game logic: To add up the score if buttons are pressed at the correct time and save scores(If there is a new high score) 

##Youtube Link
CS 120B Project Youtube [Link] (https://www.youtube.com/watch?v=acBHFVkrooQ)

## Known Bugs and Shortcomings
*The main bug that is present is that the player can press the button multiple times on a single note and gain extra points than necessarily intended.
*I tested a scoring system where it adds the points as long as you hold the button and if it was the correct time to press the button. The scores incremented very fast and accurately but the problem was that it would slow down the music notes.

## Future work
*I would have enjoyed to extend the song that I have right now and add more songs, it is only a portion of it due to the time constraints I had.
*I would have added at least one more matrix to have a better visual effect
*Also I would have added an extra LCD screen that would measure the health of the player and it would either increase or decrease the health if the player correctly pressed the button on time or not respectively.
*Having the ability to press two buttons at the same time
