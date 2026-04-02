LATEXMK ?= latexmk
MAIN ?= main.tex

.PHONY: all pdf watch clean distclean

all: pdf

pdf:
	$(LATEXMK) -r latexmkrc $(MAIN)

watch:
	$(LATEXMK) -pvc -r latexmkrc $(MAIN)

clean:
	$(LATEXMK) -r latexmkrc -c $(MAIN)

distclean:
	$(LATEXMK) -r latexmkrc -C $(MAIN)