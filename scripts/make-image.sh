#!/bin/bash -ex

# Paths
BUILD=build
SYSTEM_IMG=$BUILD/system.img
ROOT_IMG=$BUILD/root.img
BOOT_IMG=$BUILD/boot.img
ROOT=$BUILD/root
KERNEL=$BUILD/kernel

# Config
KERNEL_DEFCONFIG=bcm2711_defconfig
KERNEL_BRANCH=rpi-6.1.y
CPUS=$(grep -c ^processor /proc/cpuinfo)

BOOT_FILES="bootcode.bin start.elf fixup.dat start4.elf fixup4.dat"
BASEURL=https://github.com/raspberrypi/firmware/raw/master/boot
for FILE in $BOOT_FILES; do
  if [ ! -e $BUILD/boot/$FILE ]; then
    mkdir -p $BUILD/boot
    wget $BASEURL/$FILE -O $BUILD/boot/$FILE
  fi
done

# Kernel
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
if [ ! -e $KERNEL ]; then
  git clone --depth 1 --single-branch -b $KERNEL_BRANCH \
    https://github.com/raspberrypi/linux $KERNEL
fi

if [ ! -e $KERNEL/.config ]; then
  echo Configuring kernel
  make -C $KERNEL $KERNEL_DEFCONFIG KERNEL=kernel8
fi

if [ ! -e $KERNEL/arch/$ARCH/boot/Image ]; then
  echo Building kernel
  make -j$CPUS -C $KERNEL
fi

# Create boot image
BOOT_SIZE=$((100 * 1024 * 2)) # 100MiB in sectors
echo Building $BOOT_IMG

rm -f $BOOT_IMG
mkfs.vfat -n boot -S 512 -C $BOOT_IMG $(($BOOT_SIZE / 2))

KERNEL_BOOT=$KERNEL/arch/$ARCH/boot
mcopy -i $BOOT_IMG -s $BUILD/boot/* ::
mcopy -i $BOOT_IMG -s src/overlay/boot/* ::
mcopy -i $BOOT_IMG -s $KERNEL_BOOT/Image ::kernel8.img
mcopy -i $BOOT_IMG -s $KERNEL_BOOT/dts/broadcom/bcm2711-rpi-4-b.dtb ::
mmd -i $BOOT_IMG ::/overlays
mcopy -i $BOOT_IMG -s $KERNEL_BOOT/dts/overlays/*.dtbo ::/overlays/
mcopy -i $BOOT_IMG -s $KERNEL_BOOT/dts/overlays/README ::/overlays/

# Create root fs
if [ ! -e $ROOT -o ! -e $BUILD/root.success ]; then
  echo Building $ROOT

  rm -rf $ROOT
  mkdir $ROOT

  ARCH=aarch64
  sudo debootstrap --arch=arm64 --foreign \
    --components main,contrib,non-free-firmware bookworm $ROOT
  sudo cp -av /usr/bin/qemu-$ARCH-static $ROOT/usr/bin/

  touch $BUILD/root.success
fi

# Init root image
sudo cp scripts/init-rootfs.sh $ROOT/root/
sudo chroot $ROOT /bin/bash /root/init-rootfs.sh

# Install kernel modules
if [ ! -e $ROOT/lib/modules/6.1.46-v8+ ]; then
  sudo make -C $KERNEL INSTALL_MOD_PATH=../../$ROOT modules_install
fi

# Install bbctrl
BBCTRL_VERSION=$(sed -n 's/^.*"version": "\([^"]*\)",.*$/\1/p' package.json)
BBCTRL_PKG=bbctrl-$BBCTRL_VERSION.tar.bz2
sudo tar xf dist/$BBCTRL_PKG -C $ROOT/root/
sudo chroot $ROOT /bin/bash -c \
  "cd root/bbctrl-$BBCTRL_VERSION && ./scripts/install.sh"

# Create root image
echo Building $ROOT_IMG

(
  cd src/overlay/root
  sudo find . -type f -exec \
    install -D -g root -o root "{}" "../../../$ROOT/{}" \;
  sudo find . -type l -exec cp -a "{}" "../../../$ROOT/{}" \;
)
sudo chown -R 1000:1000 $ROOT/home/bbmc

ROOT_SIZE=$(sudo du -bs $ROOT | sed 's/\([0-9]*\).*/\1/')
ROOT_SIZE=$((ROOT_SIZE * 2 / 1024 / 1024))

rm -f $ROOT_IMG
dd if=/dev/zero of=$ROOT_IMG bs=1M count=0 seek=$ROOT_SIZE
mkfs.ext4 $ROOT_IMG
mkdir -p $BUILD/mnt
sudo mount -o loop $ROOT_IMG $BUILD/mnt
sudo cp -rfp $ROOT/* $BUILD/mnt/
sudo umount -d $BUILD/mnt

# Compute sizes
BOOT_START=2048
ROOT_START=$((BOOT_START + BOOT_SIZE))
ROOT_SIZE=$(stat -L --format="%s" $ROOT_IMG)
IMAGE_SIZE=$((ROOT_START * 512 + ROOT_SIZE + 35))
IMAGE_SIZE=$((IMAGE_SIZE / 1024 / 1024 + 2))

# Create image file
dd if=/dev/zero of=$SYSTEM_IMG bs=1M count=0 seek=$IMAGE_SIZE

# Partition
parted -s $SYSTEM_IMG -- unit s \
  mklabel gpt \
  mkpart boot $BOOT_START $((ROOT_START - 1)) \
  set 1 boot on \
  mkpart root $ROOT_START -34s

# Write partitions
dd if=$BOOT_IMG of=$SYSTEM_IMG conv=notrunc,fsync seek=$BOOT_START
dd if=$ROOT_IMG of=$SYSTEM_IMG conv=notrunc,fsync seek=$ROOT_START
