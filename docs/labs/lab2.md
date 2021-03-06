## LAB 2: Analog Circuitry and FFTs

### Objective

In this lab, we added different sensors to our robot, and made analog circuits
and a digital filter to interface with the Arduino. One is a microphone circuit
that detects a 660Hz whistle blow signifying the beginning of your maze mapping.
The other captures inputs from an IR sensor to detect an IR treasure blinking
at 7kHz.

## Acoustic Team

<div style="text-align:center"> <img src="../pictures/lab2/mic.jpeg"/> </div>

### Materials

  * Arduino Uno
  * Electret Microphone Amplifier - MAX4466 with Adjustable Gain
  * ~3 kΩ resistor
  * Function Generator
  * Oscilloscope
  * Tone generator phone-application

### Analysis of the microphone in the built-in system

When we started assembling our circuit, we made sure to test its different
components in order to make debugging easier in the future. The Electret Microphone was working perfectly according to the oscilloscope, since we could appreciate the difference in certain tones and noises. We obtained the following feedback after setting up an effective low-pass filter with a cut off frequency of 796 Hz using a 1µF capacitor and a ~200Ω resistor:

<div style="text-align:center"><img src="../pictures/lab2/lowPass.png"/></div>

Please, watch a quick demonstration of the functionality of our circuit so far:

<div style="text-align: center">
<iframe width="534" height="300" src="https://www.youtube.com/embed/f3Jj78DgqsM" frameborder="0" allowfullscreen></iframe>
</div>

For the purpose of the competition, we needed to amplify the signal since there is going to be a lot of background noise in Phillips Hall. We assembled a non-inverting amplifier with 10kΩ and 100kΩ, therefore having a gain of 10. However, we were not getting a signal at all, and the faculty even checked and validated our circuit before reaching any false conclusions. It was in fact a defectuous op amp, and many of the other teams were having the same issue. That is why we switched to a fully functional Electret Microphone Amplifier - MAX4466. This device already contained the desired analog filters, and an adjustable gain ranging from 4 to 26 -measured by our team using the function generator and the oscilloscope-:

* Peak-Peak Voltage at 660 Hz with microphone only: 200.0 mV
* Peak-Peak Voltage at 660 Hz with the built-in amplifier (MIN): 800 mV Amplifier gain: 4
* Peak-Peak Voltage at 660 Hz with the built-in amplifier (MAX): 5.28 V Amplifier gain: 26.4

From this lab, until the competition, we will work with such device to solve the acoustic challenges.

### Testing the Fast Fourier Transform algorithm provided by the Arduino

First of all, let's discuss why the Fast Fourier Transform Algorithm is the right way to solve our issue. A fast Fourier transform (FFT) algorithm computes the discrete Fourier transform (DFT) of a sequence, or its inverse (IFFT). Fourier analysis converts a signal from its original domain (often time or space) to a representation in the frequency domain and vice versa. An FFT rapidly computes such transformations by factorizing the DFT matrix into a product of sparse (mostly zero) factors. As a result, it manages to reduce the complexity of computing the DFT from `O(n^2)`, which arises if one simply applies the definition of DFT, to `O(nlogn)`, where n is the data size.

Now let's get into the process of using this powerful tool. We started testing the function generator by hooking it up directly to the oscilloscope and make sure it was working properly. We tested it with a 660Hz frequency, 2.55Vpp amplitude, and 1.25V offset. Here is the feedback of the experiment:

Function Generator         |  Oscilloscope
:-------------------------:|:-------------------------:
![](../pictures/lab2/functGenTest.jpg)  |  ![](../pictures/lab2/oscilloscopeTest.jpg)  

With the function generator working properly, it was time to test the FFT algorithm provided by the Arduino Library `<FFT.h>`. Before assembling the circuit together, we had to do some work to figure out where our 660Hz frequency would fall inside the output from the FFT.

The ADC clock is set by the main clock speed of the Arduino, divided by a specific pre-scalar. The Arduino's clock frequency is 16MHz, and the pre-scalar set by the ADC inside the code `ADCSRA = 0xe5; // set the ADC to free running mode` is 32. If we divide 16MHz / 32, we obtain a clock frequency of 500KHz for the ADC. For it to convert an analog signal into a very precise digital signal of 10 bits, it takes 13 entire cycles. This would be 500KHz / 13 which yields a frequency of 38.4KHz. However, the ADC divides its reading into 256 separate points, which symbolize the range of values in each frequency bin. If we divide 38.4KHz / 256, we obtain 150Hz as our bin range. Therefore, each frequency bin contain values ranging from x to x + 150Hz. Our desired frequency to detect is 660Hz, which according to a simple division of 660Hz / 150Hz should appear in either bin 4 or bin 5. With the theoretical reasoning behind it, we tested the ADC using the function generator at different frequencies, all of them integer multiples of 660Hz in order to analyze precision and to check if there was an even space between the bins. This was our result:

<div style="text-align:center"><img src="../pictures/lab2/freqs.png"/></div>

And this is the data that we obtained from the graph regarding different bins and frequencies `(Frequency:Bin)` :

(660: 5), (1320: 10), (1980: 14), (2640: 19), (3300: 23), (3960: 27), (4620: 32), (5280: 36)

Our feedback was what we expected! We not only read the 660Hz in bin 5, but we could actually see the even spacing between the bins of the frequencies multiples of 660Hz -as it was expected as well, of course-. With that out of the way, we proceeded to actually test the circuit with our own 660Hz tone.

### Testing the circuit with a 660 Hz tone

Before doing any further analysis of the signal, or even editing the code to display any data, our team did a small double-check test to see how the circuit reacted to (1) people talking in the background, (2) a random song, and (3) the 660Hz tone generated from the mobile application we downloaded. This graph clearly portrays the result:

<div style="text-align:center"><img src="../pictures/lab2/comparison.png"/></div>

At the dotted spot, we can see how the 5th frequency bin is at its peak in the 660Hz tone graph, while the other two are really low at that point. This tells us three things:
1. Our circuit is definitely recognizing the 660Hz above all.
2. The signal can get tampered by outside noise, so we cannot rely on the reading at ONE specific point in time. We need a *moving average algorithm*!
3. We need to work on filtering the signal even more so that we can set it apart from other relatively close signals -such as 585Hz (bin 4) or 735Hz (bin 6)-.

#### Moving average

A moving average is a succession of averages derived from successive segments (typically of constant size and overlapping) of a series of values. Here is how we accomplished ours:

```c
// Moving average variables
const int numReadings = 10;

int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average


// Moving average algorithm --------------------------------------

// subtract the last reading:
total = total - readings[readIndex];

// read from the sensor:
readings[readIndex] = fft_log_out[4]; // fft_log_out[4] represents the 5th bin being outputted by the ADC

// add the reading to the total:
total = total + readings[readIndex];

// advance to the next position in the array:
readIndex = readIndex + 1;

// if we're at the end of the array...
if (readIndex >= numReadings) {
  // ...wrap around to the beginning:
  readIndex = 0;
}

// calculate the average:
average = (int)(total / numReadings);
```

We definitely saw an improvement regarding the stability of the signal readings, since it was not fluctuating so much as before. Now it was time to focus on the 3rd point that was mentioned above.

### Distinguish a 660Hz tone (from tones at 585Hz and 735Hz)

When testing our circuit to see if it could recognize the different frequencies (585Hz, 660Hz, 735Hz), we stumbled across an interesting issue. The circuit was activated with the three frequencies instead of just 660Hz! And we could see why: 585Hz, 660Hz, and 735Hz fall within -or really close- the same 5th frequency bin. Refer to the following picture for the data recollected:

<div style="text-align:center"><img src="../pictures/lab2/uneditFreqs.png"/></div>

The three frequencies definitely fall really close to each other, so it expected that the circuit does not recognize each of them individually. We came up with a solution that was very effective and it involved the slowing of the ADC clock. In the previous sections, we had pre-scalar of 32 and that was giving us a bins with a frequency range of approximately 150Hz. We cannot afford to keep working like this, since this 150Hz range contains -or is really close to- the three frequencies above. Our solution was to alter the pre-scalar by changing the following line in the Arduino: `ADCSRA = 0xe5;` to `ADCSRA = 0xe7;`. This sets the value of the pre-scalar to 128, which yields the following result: 16MHz / 256 points / 13 cycles / 128 ps = 37.56Hz frequency range for each bin. This result is great! With a smaller range, our three frequencies will fall more apart from each other and therefore will be recognized independently by the Arduino. Refer to the following graph for the new circuit performance:

<div style="text-align:center"><img src="../pictures/lab2/editFreqs.png"/></div>

In this case, 585Hz falls in bin 16, 660Hz falls in bin 18, and 785Hz falls in bin 20. To separate them even more, we came up with a simple, yet effective idea to make our signal stronger: a digital filter. Although it may sound hard, it is just a mathematical algorithm to set apart values that are different already. This is the code that we used:

```
// Digital Filter Algorithm
average = (average - 50) * 100;
```

This expression actually created a bigger difference that expected regarding the separation of the signal. We were able to detect the 660Hz -nothing lower or higher- at a considerable distance of 10 inches! Considering that in the actual competition, the starting tone speaker will be approximately 5 inches apart and at a full volume, we can safely consider our circuit to work very effective. Please, take a look at the comparison between the three frequencies' outputs at random bins, running the function generator at 660Hz:

<div style="text-align:center"><img src="../pictures/lab2/freqComp.png"/></div>

To test it even further, we created the following function to help us visualize the reaction of the Arduino in the presence of the tone:

```c
void detectingTone(int value){
  if(value > 8000){
      Serial.println("Yes");
    }
    else{
      Serial.println("No");
    }
}
```

The variable `value` is the output of the moving average, which is compared to 8000 -a generic value determined by testing the circuit with other signals- to output either "Yes" or "No". Please, see a demonstration of the process in the following video:

<div style="text-align: center">
<iframe width="534" height="300" src="https://www.youtube.com/embed/1Al7tk_LpDg" frameborder="0" allowfullscreen></iframe>
</div>

### Conclusions

Our team has done a great job figuring out the endless possibilities to solve the issues that come along this project. We used different techniques and principles of signal analysis to not only convert analog to digital signal, but to filter them in both states. We practiced how to filter our signal using high and low pass filters, as well as digital filters. We worked with operational amplifiers to amplify our analog input, and very importantly, we had the opportunity to analyze the data obtained from the ADC inside the Arduino script. However, we still believe there is more work that can be done in order to improve the efficiency of our circuit. For instance, we are starting to consider using cascading amplifiers to obtain a relatively larger gain -two amplifiers of gain 10 each, for example-. Also, we also want to create a sort of band pass filter to limit our circuit to obtain data from a certain range of frequencies and ignore all remaining noise. It will all be determined in the future, depending on the feedback we receive from intense testing sessions.

## IR Sensor Team


<div style="text-align:center"> <img src="../pictures/lab2/IR_Team.png"/> </div>

### Materials

  * Arduino Uno
  * NPN IR Photo Transistor
  * LM358P Op Amp
  * 2.2kΩ resistor
  * 10kΩ resistor
  * 22kΩ resistor
  * 2x 330Ω resistors
  * Green LED
  * Red LED
  * Function Generator
  * Oscilloscope
  * Treasure Beacon

### Treasure Signal Unamplified

We initially set up a basic circuit to measure the output of the IR phototransistor consisting of the phototransistor connected in series with a resistor between a 5V power supply and ground. We measured the output voltage as the voltage drop across the resistor. The output voltage varies based on the amount of current passing through the phototransistor. Without amplification, we had output voltages ranging from ~500 mV at 2 inches between the treasure and the phototransistor to ~1.5 V at 0.5 inches. This signal is displayed as Ch1 in our pictures of the oscilloscope readings. While the signal was still reasonably large, we decided to amplify the output anyway to minimize the likelihood of a false positive from our treasure detection software and so that we can detect treasures from farther away.

<div style="text-align:center"> <img width="200" height="320" src="../pictures/lab2/IR_circuit.PNG"/> </div>


### Treasure Signal Amplified

We decided to use a noninverting amplifier to increase the magnitude of our output signal. We chose a noninverting amplifier as opposed to an inverting amplifier so that we would not need to worry about supplying a negative voltage to our op amp rails. For a noninverting amplifier, the gain is given by the equation:


<div style="text-align:center"> <img  width="330" height="100" src="../pictures/lab2/OpAmp_Formula.PNG"/> </div>

where R1 is the feedback resistor between the output and the inverting input and R2 is the resistor between the inverting input and ground. Since our output is already fairly large, we chose to implement our amplifier with a gain of 3. To do this, we chose the resistor values R1 = 22 kΩ and R2 = 10 kΩ.

<div style="text-align:center"> <img  width="370" height="220" src="../pictures/lab2/IR_circuit_amplifier.PNG"/> </div>

Our actual output from the amplifier circuit did not quite have the gain of 3 that we expected. The gain seemed to vary between 2 and 3 depending on the magnitude of the input. We weren’t able to determine exactly why the gain fluctuated so much, but we suspect that it is most likely due to the transistors making up the op amp. The output from the op amp is shown as Ch2 in the pictures of the oscilloscope data. While we did not feel that this issue was sufficiently important to attempt to fix for this lab, it may be worthwhile later on to switch out our op amp so that the circuit behaves a little more predictably.

The pictures below show the treasure location and the corresponding signal being picked up by the phototransistor. The yellow signal is unamplified and the blue signal is amplified. Even at 2 inches away, our amplifier was stil outputing signal pk-to-pk of 960mV. We think this setup will work well for detecing treasures from long distances (~2in).



Treasure         |  Oscilloscope
:-------------------------:|:-------------------------:
![](../pictures/lab2/Treasure_1in.jpg)  |  ![](../pictures/lab2/Scope_1in.jpg) 

Treasure         |  Oscilloscope
:-------------------------:|:-------------------------:
![](../pictures/lab2/Treasure_2in.jpg)  |  ![](../pictures/lab2/Scope_2in.jpg) 


### Detecting Treasure using FFT

Like the acoustic team, we used the Arduino FFT libraries to take the fourier transform of our input signal to determine the frequency of the IR signal. Using the same calculations described in the section “Testing the Fast Fourier Transform algorithm provided by the Arduino”, we determined that 7 kHz would appear in bin 46 or 47, 12 kHz would appear in bin 80, and 17 kHz would appear in bin 113 or 114. For the purposes of this lab, we only focused on treasures set at 7 kHz and 12 kHz. The FFT output from the Arduino confirmed that our calculations were correct, as shown in the graphs below.

:-------------------------:|:-------------------------:
![](../pictures/lab2/12kHz_Sweep.PNG)  |  ![](../pictures/lab2/7kHz_Sweep.PNG)

To determine whether we had detected the treasure, we compared the magnitude of the FFT output in the bin corresponding to our desired frequency to the magnitude of the FFT output in the bins to either side of that frequency. When the treasure is present, we expect the FFT to have a much larger magnitude at the bin the treasure frequency is in than in the surrounding bins. For instance, to detect the 7 kHz signal, we compared the value in bin 47 to the values in bins 44 and 49. If bin 47 contained a value with a magnitude at least 20 greater than both of the other two bins, we determined that the 7 kHz treasure was present. We chose a threshold of 20 because it was large enough to encompass the fluctuations due to noise but not so large that it would fail to detect a signal of ~800 mV.


```
// Filtering for high magnitude in bin 47 and bin 80
    bool top1 = (fft_log_out[47] - fft_log_out[44]) > 20;
    bool bottom1 = (fft_log_out[47] - fft_log_out[49]) > 20;

    bool top2 = (fft_log_out[80] - fft_log_out[78]) > 20;
    bool bottom2 = (fft_log_out[80] - fft_log_out[82]) > 20;


///Detecting LEDs
    if (top1 && bottom1){ // 7khz signal because bin 40 is a spike
      digitalWrite(led1, HIGH);}   // turn the LED on (HIGH is the voltage level)
    else{
      digitalWrite(led1, LOW);}
    
    if (top2 && bottom2){  // 12khz because bin 80 is a spike
      digitalWrite(led2, HIGH);}   // turn the LED on (HIGH is the voltage level)
    else{
      digitalWrite(led2, LOW);}
```


### Distinguishing between 7kHz and 12kHz 

For testing purposes, we wrote an Arduino program that would detect treasures at 7 kHz and 12 kHz and light up different LEDs corresponding to which frequency was detected. When we detect a 7 kHz signal, we light up a green LED. When we detect a 12 kHz signal, we light up a red LED. A demonstration of the treasure detection is shown in the following video.

<div style="text-align: center">
<iframe width="534" height="300" src="https://www.youtube.com/embed/Q39TYC1IMCU" frameborder="0" allowfullscreen></iframe>
</div>


### Conclusions

We were able to successfully implement an IR detection circuit that would reliably detect the treasure at ~2 inches away from the phototransistor. We have some lingering quirks, primarily the gain that is fluctuating with the magnitude of the input, but these are issues that can be dealt with later with minimal effort if necessary. Our code is able to distinguish between the two different treasures and can be easily expanded to include detection for the 17 kHz treasure. We may want to look into including filters in our circuit later to minimize the effects of random IR noise if further testing suggests that noise may become an issue.
