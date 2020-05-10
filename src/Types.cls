global @Point {
	int x;
	int y;
}

global @Rect {
	int left;
	int bottom;
	int right;
	int top;
}

global @Box
{
	int x;
	int y;
	int width;
	int height;
}

global %Font with utf8 {
	int         height            with undef;
	int         width             with undef;
	int         style             with undef;
	int         pitch             with undef;
	double      direction         with undef;
	int         vector            with undef;
	string      name              with undef with utf8(FONT_UTF8_NAME);
	int         size              with undef;
	string      encoding          with undef;
	long        resolution        ;
	string      family            with utf8(FONT_UTF8_FAMILY);
	int         ascent            ;
	int         descent           ;
	int         weight            ;
	int         maximalWidth      ;
	int         internalLeading   ;
	int         externalLeading   ;
	int         xDeviceRes        ;
	int         yDeviceRes        ;
	int         firstChar         ;
	int         lastChar          ;
	int         breakChar         ;
	int         defaultChar       ;
}

global @FillPattern U8[8];

global @NPoint {
	double x;
	double y;
}

global %PrinterInfo {
	string name;
	string device;
	Bool   defaultPrinter;
}
