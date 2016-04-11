TEXS=$(wildcard *.tex)
PDFS=$(patsubst %.tex,%.pdf,$(TEXS))

.PHONY: all
all: $(PDFS)

proposal.pdf: refs.bib

build/%.aux: %.tex
	mkdir -p build
	pdflatex -output-directory build/ $<

%.pdf: %.tex %.bbl
	pdflatex -output-directory build/ $<
	pdflatex -output-directory build/ $<
	cp build/*.pdf .

%.bbl: build/%.aux
	bibtex $<

.PHONY: clean
clean:
	rm -rf *.aux *.log _region_* *.snm *.nav *.out *.toc
	rm -rf build/
