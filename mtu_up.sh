#! /bin/bash
ethtool -C enp1s0 adaptive-rx on
ethtool -C enp1s0 adaptive-tx on
sudo ifconfig enp1s0 down
sudo ifconfig enp1s0 mtu 9000
sudo ifconfig enp1s0 up
