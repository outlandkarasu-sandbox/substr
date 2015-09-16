# makefile for substr and ranstr

.PHONY: build clean test testfiles

testvolumes=100 1000 10000 100000 1000000 10000000
testfiles=$(addprefix dataset/,$(addsuffix .txt,$(testvolumes)))

build: substr ranstr testfiles clean
testfiles: $(testfiles)

substr.o : substr.c
ranstr.o : ranstr.c

substr : substr.o
ranstr : ranstr.o

dataset/%.txt:; ./ranstr $(patsubst dataset/%.txt,%,$@) > $@

clean:
	@rm -f substr.o ranstr.o

