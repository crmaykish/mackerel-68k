echo "Installing required packages for Mackerel-68k build tools"

sudo pacman -S --noconfirm base-devel wget unzip git bc rsync linux-headers ncurses

# genromfs builds the Mackerel-08 ROMfs root filesystem. It is only in the AUR,
# so it can't be installed with plain pacman.
if command -v yay >/dev/null; then
	yay -S --noconfirm genromfs
elif command -v paru >/dev/null; then
	paru -S --noconfirm genromfs
else
	echo "NOTE: genromfs is required to build the Mackerel-08 root filesystem but is"
	echo "      only available in the AUR. Install it with an AUR helper, e.g.:"
	echo "          yay -S genromfs"
fi
