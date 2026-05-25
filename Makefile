all:
	cmake -B build-mac -S .
	cmake --build build-mac
	cp build-mac/app .

clean:
	rm -rf build-mac app src/*.o

