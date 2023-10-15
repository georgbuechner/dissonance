#Variables 
AUBIO_VERSION=0.4.7
AUBIO_FRAMEWORK=aubio_frame_work_$(AUBIO_VERSION)
UNAME_S := $(shell uname -s)

aubio:
	git submodule update --init --recursive
	cd aubio && make
	cd aubio && ./waf configure && ./waf build 
	cd aubio && sudo ./waf install

build:
	# Create build folder and install conan-dependencies.
	rm -rf build; mkdir build
ifeq ($(UNAME_S),Linux)
	conan profile new default --detect  || true 
	conan profile update settings.compiler.libcxx=libstdc++11 default
endif
	cd build && conan install .. --build=missing
	# Build projekt
	cd build && cmake ..
	cd build && make -j

setup:
	rm -rf ~/.dissonance; mkdir -p ~/.dissonance
	cp -r dissonance/. ~/.dissonance/
	chmod +x build/bin/dissonance
	sudo cp build/bin/dissonance /usr/local/bin/dissonance_bin
	chmod +x dissonance.sh
	sudo cp dissonance.sh /usr/local/bin/dissonance
	# For macos move terminfo
ifeq ($(UNAME_S),Darwin)
	mkdir -p ~/.terminfo/
	sudo cp -r /usr/share/terminfo/78 ~/.terminfo/
endif

install:
	make build
	make setup

uninstall_aubio:
	cd aubio && ./waf uninstall

uninstall_dissonance: 
	sudo rm /usr/local/bin/dissonance
	sudo rm /usr/local/bin/dissonance
	rm -rf build/
	rm -rf ~/.dissonance

uninstall:
	make uninstall_dissonance
	make uninstall_aubio
