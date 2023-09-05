if [[ -z $DISPLAY ]] && [[ $(tty) = /dev/tty1 ]]; then
  exec startx >~/.xorg.log 2>&1
fi

if [ -f ~/.bashrc ]; then
  . ~/.bashrc
fi
