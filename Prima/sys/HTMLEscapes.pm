package Prima::sys::HTMLEscapes;

use strict;
use warnings;
use base qw(Exporter);
our @EXPORT    = q(%HTML_Escapes);
our @EXPORT_OK = q(%HTML_Escapes);
our %HTML_Escapes = (
	'amp'         =>        '&',           #   ampersand
	'lt'          =>        '<',           #   left chevron, less-than
	'gt'          =>        '>',           #   right chevron, greater-than
	'quot'        =>        '"',           #   double quote

	"Aacute"      =>        "\xC1",        #   capital A, acute accent
	"aacute"      =>        "\xE1",        #   small a, acute accent
	"Acirc"       =>        "\xC2",        #   capital A, circumflex accent
	"acirc"       =>        "\xE2",        #   small a, circumflex accent
	"AElig"       =>        "\xC6",        #   capital AE diphthong (ligature)
	"aelig"       =>        "\xE6",        #   small ae diphthong (ligature)
	"Agrave"      =>        "\xC0",        #   capital A, grave accent
	"agrave"      =>        "\xE0",        #   small a, grave accent
	"Aring"       =>        "\xC5",        #   capital A, ring
	"aring"       =>        "\xE5",        #   small a, ring
	"Atilde"      =>        "\xC3",        #   capital A, tilde
	"atilde"      =>        "\xE3",        #   small a, tilde
	"Auml"        =>        "\xC4",        #   capital A, dieresis or umlaut mark
	"auml"        =>        "\xE4",        #   small a, dieresis or umlaut mark
	"Ccedil"      =>        "\xC7",        #   capital C, cedilla
	"ccedil"      =>        "\xE7",        #   small c, cedilla
	"Eacute"      =>        "\xC9",        #   capital E, acute accent
	"eacute"      =>        "\xE9",        #   small e, acute accent
	"Ecirc"       =>        "\xCA",        #   capital E, circumflex accent
	"ecirc"       =>        "\xEA",        #   small e, circumflex accent
	"Egrave"      =>        "\xC8",        #   capital E, grave accent
	"egrave"      =>        "\xE8",        #   small e, grave accent
	"ETH"         =>        "\xD0",        #   capital Eth, Icelandic
	"eth"         =>        "\xF0",        #   small eth, Icelandic
	"Euml"        =>        "\xCB",        #   capital E, dieresis or umlaut mark
	"euml"        =>        "\xEB",        #   small e, dieresis or umlaut mark
	"Iacute"      =>        "\xCD",        #   capital I, acute accent
	"iacute"      =>        "\xED",        #   small i, acute accent
	"Icirc"       =>        "\xCE",        #   capital I, circumflex accent
	"icirc"       =>        "\xEE",        #   small i, circumflex accent
	"Igrave"      =>        "\xCD",        #   capital I, grave accent
	"igrave"      =>        "\xED",        #   small i, grave accent
	"Iuml"        =>        "\xCF",        #   capital I, dieresis or umlaut mark
	"iuml"        =>        "\xEF",        #   small i, dieresis or umlaut mark
	"Ntilde"      =>        "\xD1",        #   capital N, tilde
	"ntilde"      =>        "\xF1",        #   small n, tilde
	"Oacute"      =>        "\xD3",        #   capital O, acute accent
	"oacute"      =>        "\xF3",        #   small o, acute accent
	"Ocirc"       =>        "\xD4",        #   capital O, circumflex accent
	"ocirc"       =>        "\xF4",        #   small o, circumflex accent
	"Ograve"      =>        "\xD2",        #   capital O, grave accent
	"ograve"      =>        "\xF2",        #   small o, grave accent
	"Oslash"      =>        "\xD8",        #   capital O, slash
	"oslash"      =>        "\xF8",        #   small o, slash
	"Otilde"      =>        "\xD5",        #   capital O, tilde
	"otilde"      =>        "\xF5",        #   small o, tilde
	"Ouml"        =>        "\xD6",        #   capital O, dieresis or umlaut mark
	"ouml"        =>        "\xF6",        #   small o, dieresis or umlaut mark
	"szlig"       =>        "\xDF",        #   small sharp s, German (sz ligature)
	"THORN"       =>        "\xDE",        #   capital THORN, Icelandic
	"thorn"       =>        "\xFE",        #   small thorn, Icelandic
	"Uacute"      =>        "\xDA",        #   capital U, acute accent
	"uacute"      =>        "\xFA",        #   small u, acute accent
	"Ucirc"       =>        "\xDB",        #   capital U, circumflex accent
	"ucirc"       =>        "\xFB",        #   small u, circumflex accent
	"Ugrave"      =>        "\xD9",        #   capital U, grave accent
	"ugrave"      =>        "\xF9",        #   small u, grave accent
	"Uuml"        =>        "\xDC",        #   capital U, dieresis or umlaut mark
	"uuml"        =>        "\xFC",        #   small u, dieresis or umlaut mark
	"Yacute"      =>        "\xDD",        #   capital Y, acute accent
	"yacute"      =>        "\xFD",        #   small y, acute accent
	"yuml"        =>        "\xFF",        #   small y, dieresis or umlaut mark

	"lchevron"    =>        "\xAB",        #   left chevron (double less than)
	"rchevron"    =>        "\xBB",        #   right chevron (double greater than)
);


1;
