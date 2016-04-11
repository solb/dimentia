TEXS=$(wildcard *.tex)
PDFS=$(patsubst %.tex,%.pdf,$(TEXS))

all: $(PDFS)

proposal.pdf: refs.bib

%.pdf: %.tex
	mkdir -p build
	pdflatex -output-directory build/ $<
	bibtex build/*.aux
	pdflatex -output-directory build/ $<
	pdflatex -output-directory build/ $<
	cp build/*.pdf .

clean:
	rm -rf *.aux *.log _region_* *.snm *.nav *.out *.toc
	rm -rf build/
