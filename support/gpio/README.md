# GPIO device permissions

`99-astropowerbox-gpio.rules` grants the `gpio` group read/write access to
GPIO character devices such as `/dev/gpiochip0`. It avoids granting GPIO
access to every local user.

Install the rule and add the account that runs AstroPowerBox to the `gpio`
group:

```sh
sudo install -D -m 0644 support/gpio/99-astropowerbox-gpio.rules \
  /etc/udev/rules.d/99-astropowerbox-gpio.rules
sudo groupadd --force gpio
sudo usermod -aG gpio "$USER"
sudo udevadm control --reload-rules
sudo udevadm trigger --subsystem-match=gpio
```

Start a new login session after changing group membership. Existing GPIO
devices should then be group-owned by `gpio` with mode `0660`, for example:

```text
crw-rw---- 1 root gpio ... /dev/gpiochip0
```
