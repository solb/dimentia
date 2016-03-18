TEXS=$(wildcard *.tex)
PDFS=$(patsubst %.tex,%.pdf,$(TEXS))

all: $(PDFS)

%.pdf: %.tex
	mkdir -p build
	pdflatex -output-directory build/ $<
	cp build/*.pdf .

clean:
	rm -rf *.aux *.log _region_* *.snm *.nav *.out *.toc
	rm -rf build/
