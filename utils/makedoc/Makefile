# $Id$
all: Prima.dvi Prima.pdf

Prima.dvi: Prima.tex
	latex Prima.tex
	latex Prima.tex

Prima.ps: Prima.dvi
	dvips -Ppdf -o Prima.ps Prima.dvi

Prima.pdf: Prima.ps
	ps2pdf -dAutoFilterColorImages=false -dColorImageFilter=/FlateEncode -dAutoFilterGrayImages=false -dGrayImageFilter=/FlateEncode Prima.ps

Prima.tex: intro.tex
	perl makedoc.pl

clean:
	rm -f Prima.ps Prima.dvi Prima.pdf Prima.tex Prima.log Prima.cache.tex Prima.ind Prima.aux Prima.idx Prima.toc texput.log out.tex *.eps

view: Prima.dvi
	xdvi Prima.dvi

pdfview: Prima.pdf
	acroread Prima.pdf
