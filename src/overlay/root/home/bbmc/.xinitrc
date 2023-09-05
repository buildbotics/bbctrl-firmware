xset -dpms
xset s off
xset s noblank

while true; do
  bbkbd -b 0.5 0 -k run-browser
  sleep 1
done
