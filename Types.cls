global %Font {
   int         height            ;
   int         width             ;
   int         style             ;
   int         pitch             ;
   int         direction         ;
   long        resolution        ;
   string      name              ;
   int         size              ;
   int         codepage          ;
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
