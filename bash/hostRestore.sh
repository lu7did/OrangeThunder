#!/bin/sh
sudo tar -zxvf $(hostname).host.tar.gz -C /
sudo apt-get update
sudo apt-get install -y git pulseaudio pavucontrol mplayer nmap ntp  socat
sudo apt-get install python3-pip gnome-schedule

#sudo apt-get update

#*------
#*------ Download support for rtl-sdr  
#*------

echo "Support for rtl-sdr"
sudo apt-get install libusb-1.0-0-dev git cmake -y
git clone https://github.com/lu7did/rtl-sdr
cd rtl-sdr/
mkdir build
cd build
cmake ../ -DINSTALL_UDEV_RULES=ON
make
sudo make install
sudo cp ../rtl-sdr.rules /etc/udev/rules.d/
sudo ldconfig
echo 'blacklist dvb_usb_rtl28xxu' | sudo tee --append /etc/modprobe.d/blacklist-dvb_usb_rtl28xxu.conf
# Now reboot
