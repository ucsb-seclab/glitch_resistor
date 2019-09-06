#!/bin/sh
sudo kextunload -verbose -bundle-id com.apple.driver.AppleUSBFTDI -personalities-only
sudo kextutil -verbose -bundle-id com.apple.driver.AppleUSBFTDI -personality AppleUSBEFTDI-6010-1
