# Raspberry Pi OrangeThunder Project

## Ham radio communications, a brief introduction

Ham radio designates as QRP transmission carrying low power (5W or less) and QRPp those that are even smaller, 1W or less.

As ham radio can legally use (depending on band privileges and categories of each license) up to 1.5 KW (1500 W) the 
QRP power, and even further the QRPp power, looks as pointless and not usable.

This is far from the truth.  Really small power can be enough to sustain communications over great distances and therefore being a per se
limiting factor to communicate, however  there might other factors in the basic implementation which makes enables or makes it
difficult to communicate for other reasons. Antenna setup, transmission mode and propagation conditions can be counted among some
of these factors.

Great distances can be achieved using the HF spectrum (3 to 30 MHz) where most of the traditional ham bands are located; local coverage
can be achieved using VHF (30 to 300 MHz), UHF (300 to 3000 MHz) and Microwaves (3000 MHz and beyond) where QRP or less power is the norm
but essentially the communication happens within the, pretty much, observable horizon.

Bands below HF, such as medium frecuencies (300 KHz to 3 Mhz) or even lower (less than 300 KHz) requires power well above the QRP levels
and/or combined with very specialized antennas.

Having HF as the target band for the project the higher the frequency the larger the distance which can be achieved with QRP power levels,
however, the hight the frequency the more dependent the actual propagation is from the moment of the day and solar conditions, A great
deal of information about this topic can be obtained from [here](http://www.arrl.org/propagation-of-rf-signals).

At the ham bands communications can be carried using different modes, which is the way the information is "encoded" in the signal. The
most obvious way is just talking, called Phone in ham parlance. In ham bands Phone communications is carried out using SSB (USB or LSB
depending on the band), in some higher bands narrow band FM is also used (28 MHz) but this mode is fairly more popular in VHF and 
beyond rather than HF. Phone conversations can also be carried using amplitude modulation (AM) like the one used by broadcast radios
but this mode, even if still used, is relatively inefficient for lower powers and trend not to be much used. Modes not carrying actual
voice are also fairly popular. The oldest and more traditional is CW where information is sent using Morse code, to many the most 
efficient mode of communication since it requires very low powers to achieve long distance communications, this situation is actually
being changed with the introduction of Weak Signal Modes (such as FT8) where highly specialized coding techniques are used to carry
small amoount of informations using incredibly low powers and still been able to be decoded in the other side. The modes hams uses
for communications are many to be discussed in detail here, further information can be obtained from [here](http://www.arrl.org/modes-systems). 

Finally, for a short introduction at least, an antenna component is required to carry ham radio communications. A very specialized
topic by itself and a field of endless experimentation. More information about this topic can be obtained from [here](http://www.arrl.org/building-simple-antennas).

In order to use amateur radio bands for listening the activity is free and without requirements, in most parts of the world at least,
check your local communications authorities requirements for the subject to find any condition to met. In the HF bands persons 
doing that activities are close relatives of Hams, usually called SWL (Short Wave Listerners) and for many it's a very specialized
hobby, which by the way isn't incompatible with being a Ham at the same time. See [here](https://www.dxing.com/swlintro.htm). 

To carry amateur radio comunications, the radioamateurs or "hams", needs to both receive and transmit at designated frequencies, which are called "ham bands",
although ham frequencies "allocations" are somewhat generic for that particular use, each communication authority at each band allow individuals to access
certain segments based on their license level or "privileges", typically associated with demonstrated technical ability, proof of mastery of certain aspects
of ham radio or age in the activity. Certains part of the bands are, at the same time, designated to carry specific modes in them; although the general 
assignment is made at the global level each administration usually introduces a fine tuning on that usage based on their own spectrum management policies.
As an example ham radio frequency assignments or "band plan" for USA can be seen [here](http://www.arrl.org/band-plan) but remember to check the ones
specific for your country.

## Ham radio Pixie Transceiver controlled by a Raspberry Pi board

Digital signal processing [here]([here](https://dspguru.com/dsp/tutorials/) techniques applied to radio communications are normally named
software defined radio, further information [here](https://www.ndsu.edu/fileadmin/conferences/cybersecurity/Slides/saunders_presentation.pdf).

Some recent technology breakthrus has been made quite recently that enables very sophisticated projects to be made by individual 
experimenters, in particular for this project the following combination:

* Very cheap digital generic purpose receivers such as the RTL-SDR dongle, see the RTL-SDR site [here](https://www.rtl-sdr.com/).
* Very powerful yet affordable high performance embedded boards able to execute Linux, see the Raspberry Pi site  [here](https://www.raspberrypi.org/). 
* Direct frequency synthesis libraries enabling the Raspberry Pi to act as a RF generator on frequencies from 0 to 1000 MHz and beyond, see the librpitx site [here](https://github.com/F5OEO/librpitx).

The Orange Thunder project is a technology implementation looking to act as a proof of concept of merging the above technologies, glue them together with many
public domain libraries and developing some structure code to glue all together into implementing a ham radio transceiver with the following general
spec.

* Specialized for the 20m (14 MHz band) but able to be implemented with TAPR filter/amplification boards to any other band.
* Able to operate in most ham usual modes such as CW, SSB, PSK31, FM, SSTV, RTTY and, specially, in FT8.
* Low consumption, able to be feed with a single +5V cc@2A power supply.
* Relatively simple to implement, simple additional circuitry with common parts needed.
* A platform for learning and further experimenting thru its release under a  Open Source.
* Headless operation (minimum hardware controls) thru software such as FLRig or rigctl.
* Can operate stand-alone or paired with another computer acting like a standard transceiver.
* Support CAT operations with a limited emulation of a Yaesu FT817 protocol.
* External soundcard support for voice modes (Phone, SSB) or externally generated modes.
* Forgiving and tolerant implementation, avoiding novice catastrophic mistakes turning them into learning experiences.
* Hopefully, a platform to get some well deserved fun.

This statement of specs will be implemented slowly over time, progress will be checked at this repository.

## PixiePi, Sister project

A very similar project, based on the same foundations is named [PixiePi](http://www.github.com/lu7did/PixiePi), software
libraries created for it are used on this project, and viceversa. The main difference among projects is the 
hardware used to implement the transceiver.

Main features and differences of PixiePi compared with OrangeThunder

* The receiver chain is a double conversion receiver and the transmitter chain a CW (class C) amplifier, based on the DIY kit Pixie.
* It is based on a Raspberry Pi Zero board which is substancially less powerful than a Raspberry Pi 3 or 4 board.
* It is intended to be used pretty much as a stand alone transceiver as it lack resources to host a user and GUI usage.
* Requires a 12V power supply
* Mostly headed toward CW usage although other modes are supported.
* Allows a local control with LCD display, tuniing know and other hardware.  

## DISCLAIMER

This is a pure, non-for-profit, project being performed in the pure ham radio spirit of experimentation, learning and sharing.
This project is original in conception and has a significant amount of code developed by me, it does also being built up on top
of a giant effort of others to develop the base platform and libraries used.
Therefore this code should not be used outside the limits of the license provided and in particular for uses other than
ham radio or similar experimental purposes.
No fit for any purpose is claimed nor guaranteed, use it at your own risk. The elements used are very common and safe, the skills
required very basic and limited, but again, use it at your own risk.
Despite being a software enginering professional with access to technology, infrastructure and computing facilities of different sorts
I declare this project has been performed on my own time and equipment.

## Fair and educated warning

Raspberry Pi is a marvel.

Hamradio is the best thing ever invented.

¡So don't ruin either by connecting GPIO04 directly to an antenna!

You'll make life of others in your neighboor unsormountable, and even
could get your Raspberry Pi fried in the process.

Google "raspberry pi rpitx low pass filter" to get some good advice on what to put between your Raspberry and your antenna
Or go to https://www.dk0tu.de/blog/2016/05/28_Raspberry_Pi_Lowpass_Filters/ for very good and easy to implement ideas

Remember that most national regulations requires the armonics and other spurious outcome to be -30 dB below the fundamental.


                     *********************************************************
                     *  Unfinished - incompletely published code - WIP       *
                     *  No support, issues or requests can be honored until  *
                     *  the project stabilizes. Check for updates here       *
                     *********************************************************

Most scripts arguments can be obtained using either -h o --h as argument, or just execute without them

## Pre-requisites


Most pre-requisites are described on the excellent tutorial "Setting up a low cost QRP monitoring station with an RTL-SDR V3 and Raspberry Pi 3" [here](https://www.rtl-sdr.com/tutorial-setting-up-a-low-cost-qrp-ft8-jt9-wspr-etc-monitoring-station-with-an-rtl-sdr-v3-and-raspberry-pi-3/)

### RTL-SDR Drivers

### PulseAudio and MPlayer

### CSDR

### ncat

### ntp Daemon

### WSJT-X

### Audio Piping Setup

### snd-aloop configuration

### socat

## Installation / update:

Download and compile code:
```
sudo rm -r /home/pi/OrangeThunder
cd /home/pi
git clone https://github.com/lu7did/OrangeThunder
cd OrangeThunder/src
sudo make
sudo make install
```


## Utility software

### WSJT-X

### FLRig

### FLDIGI

### HamLib/rigctl

# Program usage


```
#!/bin/sh
#*-----------------------------------------------------------------------
#* OT.sh
#* Script to implement a SSB transceiver using the OrangeThunder program
#* A remote pipe is implemented to carry CAT commands
#* Sound is feed thru the arecord command (PulseAudio ALSA), proper hardware
#* interface needs to be established. (-a parameter enables VOX)
#* the script needs to be called with the frequency in Hz as a parameter
#*            ./OT.sh [frequency in Hz]
#*-----------------------------------------------------------------------
clear
echo "Orange Thunder based SSB transceiver ($date)"
echo "Frequency defined: $1"
socat -d -d pty,raw,echo=0,link=/tmp/ttyv0 pty,raw,echo=0,link=/tmp/ttyv1 &
PID=$!
echo "Pipe for /tmp/ttyv0 PID($PID)"
#*----------------------------------------*
#* Transceiver execution using loopback   *
#*----------------------------------------*
arecord -c1 -r48000 -D hw:Loopback -fS16_LE - | sudo /home/pi/OrangeThunder/bin/OT -i /dev/stdin -s 6000 -p /tmp/ttyv1 -f "$1" -a
echo "Removing /tmp/ttyv0 PI($PID)"
sudo pkill socat
#*----------------------------------------*
#*           End of Script                * 
#*----------------------------------------*
```



# General information

## Radio licensing / RF:

  In order to transmit legally, a HAM Radio License is REQUIRED for running
  this experiment. The output is a square wave so a low pass filter is REQUIRED.
  Connect a low-pass filter (via decoupling C) to GPIO4 (GPCLK0) and Ground pin
  of your Raspberry Pi, connect an antenna to the LPF. The GPIO4 and GND pins
  are found on header P1 pin 7 and 9 respectively, the pin closest to P1 label
  is pin 1 and its 3rd and 4th neighbour is pin 7 and 9 respectively. See this
  link for pin layout: http://elinux.org/RPi_Low-level_peripherals
  
  Examples of low-pass filters can be found here:
    [here](http://www.gqrp.com/harmonic_filters.pdf)

## TAPR kits

  TAPR makes a very nice shield for the Raspberry Pi that is pre-assembled,
  performs the appropriate filtering for the 20m band, and also increases
  the power output to 20dBm! Just connect your antenna and you're good-to-go!
    [TAPR kit](https://www.tapr.org/kits_20M-wspr-pi.html)

  The expected power output is 10mW (+10dBm) in a 50 Ohm load. This looks
  neglible, but when connected to a simple dipole antenna this may result in
  reception reports ranging up to several thousands of kilometers.

  As the Raspberry Pi does not attenuate ripple and noise components from the
  5V USB power supply, it is RECOMMENDED to use a regulated supply that has
  sufficient ripple supression. Supply ripple might be seen as mixing products
  products centered around the transmit carrier typically at 100/120Hz.

  DO NOT expose GPIO4 to voltages or currents that are above the specified
  Absolute Maximum limits. GPIO4 outputs a digital clock in 3V3 logic, with a
  maximum current of 16mA. As there is no current protection available and a DC
  component of 1.6V, DO NOT short-circuit or place a resistive (dummy) load
  straight on the GPIO4 pin, as it may draw too much current. Instead, use a
  decoupling capacitor to remove DC component when connecting the output dummy
  loads, transformers, antennas, etc. DO NOT expose GPIO4 to electro- static
  voltages or voltages exceeding the 0 to 3.3V logic range; connecting an
  antenna directly to GPIO4 may damage your RPi due to transient voltages such
  as lightning or static buildup as well as RF from other transmitters
  operating into nearby antennas. Therefore it is RECOMMENDED to add some form
  of isolation, e.g. by using a RF transformer, a simple buffer/driver/PA
  stage, two schottky small signal diodes back to back.

## TX Timing:

  In weak modes some sort of additional time sync is needed, the usage of
  the ntpd package is recommended.

  As of 2017-02, NTP calibration is enabled by default and produces a
  frequency error of about 0.1 PPM after the Pi has temperature stabilized
  and the NTP loop has converged.

  Frequency calibration is REQUIRED to ensure that the WSPR-2 transmission
  occurs within the narrow 200 Hz band. The reference crystal on your RPi might
  have an frequency error (which in addition is temp. dependent -1.3Hz/degC
  @10MHz). To calibrate, the frequency might be manually corrected on the
  command line or a PPM correction could be specified on the command line.

###  NTP calibration:
  NTP automatically tracks and calculates a PPM frequency correction. If you
  are running NTP on your Pi, you can use the --self-calibration option to
  have this program querry NTP for the latest frequency correction before
  each WSPR transmission. Some residual frequency error may still be present
  due to delays in the NTP measurement loop and this method works best if your
  Pi has been on for a long time, the crystal's temperature has stabilized,
  and the NTP control loop has converged.

###  AM calibration:
  A practical way to calibrate is to tune the transmitter on the same frequency
  of a medium wave AM broadcast station; keep tuning until zero beat (the
  constant audio tone disappears when the transmitter is exactly on the same
  frequency as the broadcast station), and determine the frequency difference
  with the broadcast station. This is the frequency error that can be applied
  for correction while tuning on a WSPR frequency.

  Suppose your local AM radio station is at 780kHz. Use the --test-tone option
  to produce different tones around 780kHz (eg 780100 Hz) until you can
  successfully zero beat the AM station. If the zero beat tone specified on the
  command line is F, calculate the PPM correction required as:
  ppm=(F/780000-1)*1e6 In the future, specify this value as the argument to the
  --ppm option on the comman line. You can verify that the ppm value has been
  set correction by specifying --test-tone 780000 --ppm <ppm> on the command
  line and confirming that the Pi is still zero beating the AM station.

## Prototype

![Alt Text](docs/OrangeThunder_Prototype.jpeg?raw=true "Hardware Prototype")
![Alt Text](docs/OrangeThunder.jpg?raw=true "Hardware Prototype Schematic")


## Configuring and using WSJTX (FT8)

![Alt Text](docs/OrangeThunder_Prototype_Receiver_20200402.jpg?raw=true "WSJT-X Screenshoot operating FT8 at 20m")


### PWM Peripheral:
  The code uses the RPi PWM peripheral to time the frequency transitions
  of the output clock. This peripheral is also used by the RPi sound system
  and hence any sound events that occur during a WSPR transmission will
  interfere with WSPR transmissions. Sound can be permanently disabled
  by editing /etc/modules and commenting out the snd-bcm2835 device.


## Reference documentation:
  * [ARM](http://www.raspberrypi.org/wp-content/uploads/2012/02/BCM2835-ARM-Peripherals.pdf)
  * [BCM2835 functionality](http://www.scribd.com/doc/127599939/BCM2835-Audio-clocks)
  * [GPIO](http://www.scribd.com/doc/101830961/GPIO-Pads-Control2)
  * [VCTools](https://github.com/mgottschlag/vctools/blob/master/vcdb/cm.yaml)
  * [Kernel](https://www.kernel.org/doc/Documentation/vm/pagemap.txt)


# Credits:

  Most credits are inherited from the core package enabling this project
  which is the librpitx library by Evariste Coujard (F5EOE).

  Credits goes to Oliver Mattos and Oskar Weigl who implemented PiFM [1]
  based on the idea of exploiting RPi DPLL as FM transmitter.

  Dan MD1CLV combined this effort with WSPR encoding algorithm from F8CHK,
  resulting in WsprryPi a WSPR beacon for LF and MF bands.

  Guido PE1NNZ <pe1nnz@amsat.org> extended this effort with DMA based PWM
  modulation of fractional divider that was part of PiFM, allowing to operate
  the WSPR beacon also on HF and VHF bands.  In addition time-synchronisation
  and double amount of power output was implemented.

  James Peroulas <james@peroulas.com> added several command line options, a
  makefile, improved frequency generation precision so as to be able to
  precisely generate a tone at a fraction of a Hz, and added a self calibration
  feature where the code attempts to derrive frequency calibration information
  from an installed NTP deamon.  Furthermore, the TX length of the WSPR symbols
  is more precise and does not vary based on system load or PWM clock
  frequency.

  Michael Tatarinov for adding a patch to get PPM info directly from the
  kernel.

  Retzler András (HA7ILM) for the massive changes that were required to
  incorporate the mailbox code so that the RPi2 and RPi3 could be supported.

  * [PiFM](http://www.icrobotics.co.uk/wiki/index.php/Turning_the_Raspberry_Pi_Into_an_FM_Transmitter)
  * [WSPR for Pi](https://github.com/DanAnkers/WsprryPi)
  * [Fork created by Guido](https://github.com/threeme3/WsprryPi)
  * [This fork created by James](https://github.com/JamesP6000/WsprryPi)

Credits will be updated by the many (many) persons that created software ultimately
been used in this project.

Have fun!
73 de Pedro LU7DID
