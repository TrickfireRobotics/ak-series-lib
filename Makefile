.PHONY: clean container format test build container-build python

build: 	
	cmake -S . -B build 
	cmake --build build
	sudo cmake --install build 
#Alternate python build
python:
	cmake -S . -B build -DPYTHON_BINDINGS=ON
	cmake --build build
	sudo cmake --install build 
container:
	devcontainer up
	devcontainer exec --workspace-folder . bash
format:
	find . \( -o -path './build' \) -prune -o \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -print0 | xargs -0 clang-format -i
test: 
	./scripts/vm_test.sh
clean:
	rm -rf ./build
container-build:
	devcontainer exec --workspace-folder . -- cmake -S . -B build -DSETUP_TEST_IFNAME=ON -DBUILD_TESTING=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

