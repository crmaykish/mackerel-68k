echo "Installing required packages for Mackerel-68k build tools"

sudo apt-get update
sudo apt-get install -y build-essential flex bison libgmp3-dev libmpc-dev libmpfr-dev texinfo wget libncurses5-dev bc genromfs unzip git rsync

echo "Done!"
