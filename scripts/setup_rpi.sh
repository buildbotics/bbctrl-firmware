#!/bin/bash -e

export LC_ALL=C
cd /mnt/host

# Update the system
apt-get update
apt-get dist-upgrade -y

# Install packages
apt-get install -y avahi-daemon avrdude minicom python3-pip python3-smbus \
  i2c-tools python3-rpi.gpio libjpeg8 wiringpi
pip3 install --upgrade tornado sockjs-tornado pyserial

# Clean
apt-get autoclean

# Enable avahi
update-rc.d avahi-daemon defaults

# Change hostname
sed -i "s/raspberrypi/bbctrl/" /etc/hosts /etc/hostname

# Create bbmc user
useradd -m -p $(openssl passwd -1 buildbotics) -s /bin/bash bbmc
sed -i 's/pi$/pi,bbmc/g' /etc/group
passwd -l pi

# Disable console on serial port
sed -i 's/console=[a-zA-Z0-9]*,115200 \?//' /boot/cmdline.txt

# Disable i2c HAT ID probe
echo -n " bcm2708.vc_i2c_override=1" >> /boot/cmdline.txt

# Enable I2C
sed -i 's/#dtparam=i2c/dtparam=i2c/' /boot/config.txt
#echo 'dtparam=i2c_vc=on' >> /boot/config.txt
echo i2c-bcm2708 >> /etc/modules
echo i2c-dev >> /etc/modules

# Install bbctrl w/ init.d script
cp bbctrl.init.d /etc/init.d/bbctrl
chmod +x /etc/init.d/bbctrl
update-rc.d bbctrl defaults

# Disable Pi 3 USART BlueTooth swap
echo -e "\ndtoverlay=pi3-disable-bt" >> /boot/config.txt
rm -f /etc/systemd/system/multi-user.target.wants/hciuart.service

# Install hawkeye
dpkg -i hawkeye_0.6_armhf.deb
sed -i 's/localhost/0.0.0.0/' /etc/hawkeye/hawkeye.conf
echo 'ACTION=="add", KERNEL=="video0", RUN+="/usr/sbin/service hawkeye restart"' > /etc/udev/rules.d/50-hawkeye.rules
adduser hawkeye video

# Disable HDMI to save power and remount /boot read-only
sed -i 's/^exit 0$//' /etc/rc.local
echo "/usr/bin/tvservice -o" >> /etc/rc.local
echo "mount -o remount,ro /boot" >> /etc/rc.local
echo "gpio mode 27 alt3" >> /etc/rc.local # Enable serial CTS on pin 27
echo "exit 0" >> /etc/rc.local

# Dynamic clock to save power
echo -e "\n# Dynamic clock\nnohz=on" >> /boot/config.txt

# Shave 2 sec off of boot time
echo -e "\n# Faster boot\ndtparam=sd_overclock=100" >> /boot/config.txt

# TODO Forward 80 -> 8080 and moving bbctrl Web port

# Enable ssh
touch /boot/ssh

# Install bbctrl
tar xf bbctrl-*.tar.bz2
cd $(basename bbctrl-*.tar.bz2 .tar.bz2)
./setup.py install
