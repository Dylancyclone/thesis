#!/usr/bin/env sh
parent_path=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
cd "$parent_path"

echo "Initializing git submodules..."
git submodule update --init --recursive
echo "Installing dependencies..."
sudo pacman -S opus expat openssl alsa-lib avahi libevdev cmake base-devel --noconfirm
sudo pacman -S manjaro-arm-tools manjaro-tools-base --noconfirm
sudo getarmprofiles -f
echo "Creating symlinks..."
ln -s ../arm-profile/editions/thesis /usr/share/manjaro-arm-tools/profiles/arm-profiles/editions/thesis
ln -s ../arm-profile/overlays/thesis /usr/share/manjaro-arm-tools/profiles/arm-profiles/overlays/thesis
ln -s ../arm-profile/services/thesis /usr/share/manjaro-arm-tools/profiles/arm-profiles/services/thesis
# read -s -n 1 -p "Press any key to continue . . ."