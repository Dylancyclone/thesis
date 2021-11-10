#!/usr/bin/env sh

# The following does not work on WSL2, even in the distro is Manjaro
# case "$(uname -r)" in
#     *MANJARO*) ;;
#     *)
#         echo "This script must be run on Manjaro"
#         exit 1
#     ;;
# esac

parent_path=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; cd .. ; pwd -P )

echo "Initializing git submodules..."
git submodule update --init --recursive
echo "Installing dependencies..."
sudo pacman -S opus expat openssl alsa-lib avahi libevdev enet cmake base-devel --needed --noconfirm
sudo pacman -S manjaro-arm-tools manjaro-tools-base --needed --noconfirm
sudo getarmprofiles
echo "Creating symlinks..."
sudo ln -s $parent_path/arm-profile/editions/thesis /usr/share/manjaro-arm-tools/profiles/arm-profiles/editions/thesis
sudo ln -s $parent_path/arm-profile/overlays/thesis /usr/share/manjaro-arm-tools/profiles/arm-profiles/overlays/thesis
sudo ln -s $parent_path/arm-profile/services/thesis /usr/share/manjaro-arm-tools/profiles/arm-profiles/services/thesis
# read -s -n 1 -p "Press any key to continue . . ."