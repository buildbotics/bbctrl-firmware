#!/bin/bash -ex

export LC_ALL=C
export DEBIAN_FRONTEND=noninteractive
cd /mnt/host

# Update the system
apt-get update
apt-get dist-upgrade -y

# Install packages
echo iptables-persistent iptables-persistent/autosave_v4 boolean true | debconf-set-selections
echo iptables-persistent iptables-persistent/autosave_v6 boolean true | debconf-set-selections
apt-get install -yq avahi-daemon avrdude minicom python3-pip python3-smbus \
  i2c-tools python3-rpi.gpio libjpeg8 wiringpi dnsmasq hostapd \
  iptables-persistent chromium-browser xorg rpd-plym-splash samba \
  samba-common-bin smbclient cifs-utils ratpoison libpython3.5
pip3 install --upgrade tornado sockjs-tornado pyserial

# Clean up
apt-get autoremove -y
apt-get autoclean -y
apt-get clean

# Enable avahi
update-rc.d avahi-daemon defaults

# Change hostname
sed -i "s/raspberrypi/bbctrl/" /etc/hosts /etc/hostname

# Create bbmc user
if ! getent passwd bbmc >/dev/null; then
  useradd -m -p $(openssl passwd -1 buildbotics) -s /bin/bash bbmc
fi
adduser bbmc pi
adduser bbmc sudo
passwd -l pi

# Boot command line
cat <<EOF > /boot/cmdline.txt
root=/dev/mmcblk0p2 rootfstype=ext4 rootwait elevator=deadline fsck.repair=yes \
quiet splash logo.nologo plymouth.ignore-serial-consoles cgroup_memory=1 \
cgroup_enable=memory dwc_otg.fiq_fsm_mask=0x3 \
init=/usr/lib/raspi-config/init_resize.sh
EOF

# Boot config
cat <<EOF > /boot/config.txt
disable_overscan=1
dtparam=audio=on
disable_splash=1
initial_turbo=30
force_eeprom_read=0
dtparam=i2c_arm=on

[pi4]
dtoverlay=vc4-fkms-v3d
max_framebuffers=2

[all]
dtoverlay=pi3-disable-bt
nohz=on
max_usb_current=1
config_hdmi_boost=8
gpio=16=a3
EOF

# Enable I2C
cat <<EOF > /etc/modules-load.d/bbctrl.conf
i2c-bcm2708
i2c-dev
EOF

# Disable Pi 3 USART BlueTooth swap
rm -f /etc/systemd/system/multi-user.target.wants/hciuart.service

# Enable ssh
touch /boot/ssh

# Fix boot
sed -i 's/^PARTUUID=.*\/boot/\/dev\/mmcblk0p1 \/boot/' /etc/fstab
sed -i 's/^PARTUUID=.*\//\/dev\/mmcblk0p2 \//' /etc/fstab

# Enable browser in xorg
sed -i 's/allowed_users=console/allowed_users=anybody/' /etc/X11/Xwrapper.config
(
  echo 'Section "ServerFlags"'
  echo '  Option "DontVTSwitch" "on"'
  echo 'EndSection'
) > /etc/X11/xorg.conf

# Samba
echo -e "buildbotics\nbuildbotics" | smbpasswd -a bbmc
cat <<EOF > /etc/samba/smb.conf
[global]
netbios name = Buildbotics
server string = Buildbotics CNC Controller
wins support = yes
workgroup = WORKGROUP

[buildbotics]
path = /var/lib/bbctrl/upload
browseable = yes
read only = no
writeable = yes
create mask = 0777
directory mask = 0777
public = no
EOF

# Allow any user to shutdown
chmod +s /sbin/{halt,reboot,shutdown,poweroff}

# Install bbctrl
PKG=$(ls /mnt/host/bbctrl-*.tar.bz2 | head -1)
if [ -e "$PKG" ]; then
  PKGDIR=$(basename $PKG .tar.bz2)
  tar xf $PKG
  cd $PKGDIR
  ./scripts/install.sh --no-avr
  cd ..
  rm -rf $PKGDIR
fi

echo Setup complete
