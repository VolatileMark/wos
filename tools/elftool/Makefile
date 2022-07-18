URL = https://gitlab.com/bztsrc/osz/-/raw/master/tools/elftool.c
SRC = elftool.c

elftool: $(SRC)
	gcc -o $@ $<
	chmod +x $@

$(SRC):
	wget -O $@ $(URL)

.PHONY: clean
clean:
	rm -f $(SRC) elftool
