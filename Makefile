aubio:
	mkdir -p deps
	# Install aubio
	wget -O deps/aubio-0.4.7.tar.bz2 https://aubio.org/pub/aubio-0.4.7.tar.bz2
	sudo ln -s /usr/bin/python3 /usr/bin/python
	cd deps && tar xf aubio-0.4.7.tar.bz2
	cd deps/aubio-0.4.7 && ./waf configure build
	cd deps/aubio-0.4.7 && sudo ./waf install
	sudo cp /usr/local/lib/libaubio.so.5 /usr/lib/

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
	sudo cp build/bin/dissonance /usr/bin/

install:
	make build
	make setup

uninstall_aubio:
	cd deps/aubio-0.4.7 && ./waf uninstall
	rm -rf deps/
	sudo rm /usr/lib/libaubio.so.5

uninstall_dissonance: 
	sudo rm /usr/bin/dissonance
	rm -rf build/
	rm -rf ~/.dissonance

uninstall:
	make uninstall_aubio
	make uninstall_dissonance
