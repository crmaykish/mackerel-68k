GALASM = galasm

all: address_decoder.jed interrupt_decoder.jed dtack_decoder.jed

%.jed: %.pld
	$(GALASM) $<

clean:
	rm -f *.chp *.fus *.jed *.pin
