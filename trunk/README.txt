sudo apt-get install build-essential
sudo apt-get install libsdl1.2-dev libsdl-image1.2-dev libsdl-mixer1.2-dev libsdl-ttf2.0-dev

gcc -o conway conway.c -lSDL
./conway
