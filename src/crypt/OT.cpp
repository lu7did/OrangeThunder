/* 
 * OT.cpp
 * Raspberry Pi based USB experimental SSB Generator for digital modes (mainly WSPR and FT8)
 * Experimental version largely modelled after rtl_fm  and librpitx
 * This program tries to mimic the behaviour of simple QRP SSB transceivers
 * SSB generation by the librpitx library isn´t linear but not much different from 
 * highly compressed HF SSB voice signals, the TAPR amplifier isn´t much linear either.
 * So the transceiver is specially suited to PSK and FSK modulated modes such as WSPR,FT8, SSTV, CW or PSK31.
 *---------------------------------------------------------------------
 * This program is a sister project from a Raspberry Pi used to control
 * a Pixie transceiver hardware.
 * Project at http://www.github.com/lu7did/PixiePi
 *---------------------------------------------------------------------
 *  
 *
 * Created by Pedro E. Colla (lu7did@gmail.com)
 * Code excerpts from several packages:
 *    rtl_fm.c from Steve Markgraf <steve@steve-m.de> and others 
 *    Adafruit's python code for CharLCDPlate
 *    tune.cpp from rpitx package by Evariste Courjaud F5OEO
 *    sendiq.cpp from rpitx package (also) by Evariste Coujaud (F5EOE)
 *    wiringPi library (git clone git://git.drogon.net/wiringPi)
 *    iambic-keyer (https://github.com/n1gp/iambic-keyer)
 *    log.c logging facility by  rxi https://github.com/rxi/log.c
 *    minIni configuration management package by Compuphase https://github.com/compuphase/minIni/tree/master/dev
 *    tinyalsa https://github.com/tinyalsa/tinyalsa
 *    PJ_RPI  GPIO low level handler git clone git://github.com/Pieter-Jan/PJ_RPI.git
 * Also libraries
 *    librpitx by  Evariste Courjaud (F5EOE)
 *    libcsdr by Karol Simonyi (HA7ILM) https://github.com/compuphase/minIni/tree/master/dev
 *
 * ---------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#define  OT4D
#include "/home/pi/OrangeThunder/src/OT4D/OT4D.cpp"

