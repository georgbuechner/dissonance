# Variables 	
	AUBIO_VERSION=0.4.9


aubio:
	mkdir -p deps
	# Install aubio
	wget -O deps/aubio-$(AUBIO_VERSION).tar.bz2 https://aubio.org/pub/aubio-$(AUBIO_VERSION).tar.bz2
	cd deps && tar xf aubio-$(AUBIO_VERSION).tar.bz2
	cd deps/aubio-$(AUBIO_VERSION) && ./waf configure build
	cd deps/aubio-$(AUBIO_VERSION) && sudo ./waf install

build:
	# Create build folder and install conan-dependencies.
	rm -rf build; mkdir build
	cd build && conan install .. --build=missing -s compiler.libcxx=libstdc++11
	# Build projekt
	cd build && cmake ..
	cd build && make -j

setup:
	mkdir -p ~/.dissonance
	cp -r dissonance/. ~/.dissonance/
	chmod +x build/bin/dissonance
	sudo cp build/bin/dissonance /usr/bin/dissonance_bin
	chmod +x dissonance.sh
	sudo cp dissonance.sh /usr/bin/dissonance


install:
	make build -B
	make setup

uninstall_aubio:
	cd deps/aubio-$(AUBIO_VERSION) && ./waf uninstall
	rm -rf deps/

uninstall:
	sudo rm /usr/bin/dissonance_bin
	sudo rm /usr/bin/dissonance
	rm -rf build/
	rm -rf ~/.dissonance

