CC = clang
CXX = clang++
CXXFLAGS = -fopenmp -lm -O3 -march=native -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize
CFLAGS = -O3 -lm -fopenmp
TARGETS = hw3

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS) *.out createSampleInput

.PHONY: createSampleInput
createSampleInput:
	g++ createSampleInput.cc -o createSampleInput