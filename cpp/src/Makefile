main: support
	$(CXX) -O3 -Wall -shared -std=c++2a ../../include/opentims/opentims++/sqlite3.o ../../include/opentims/opentims++/zstddeclib.o -ldl -lpthread -fopenmp -fPIC $(python3-config --includes) -I../../include/pybind11/include -I/home/tim/miniconda3/envs/master_py3_10/include/python3.10 Proteolizard.cpp -o ../../python/libproteolizard.so -ltbb
support: ../../include/opentims/opentims++/sqlite3.o ../../include/opentims/opentims++/zstddeclib.o
