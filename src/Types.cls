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

global %Font {
	int         height            ;
	int         width             ;
	int         style             ;
	int         pitch             ;
	double      direction         ;
	long        resolution        ;
	string      name              ;
	int         size              ;
	string      encoding          ;
	string      family            ;
	int         vector            ;
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
	U8          utf8_flags        ;
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
