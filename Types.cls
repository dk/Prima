global %Font {
   int    height     ;
   int    width      ;
   int    style      ;
   int    pitch      ;
   long   resolution ;
   int    direction  ;
   string name       ;
   int    size       ;
   int    ascent     ;
   int    descent    ;
   int    codepage   ;
}

global @FillPattern U8[8];

global @NPoint {
   double x;
   double y;
}

global %FontMetric {
   string  family             ;
   string  facename           ;
   int nominalSize            ;
   int width                  ;
   int height                 ;
   int style                  ;
   int pitch                  ;
   int weight                 ;
   int maxAscent              ;
   int lcAscent               ;
   int maxDescent             ;
   int lcDescent              ;
   int averageWidth           ;
   int internalLeading        ;
   int externalLeading        ;
   int xDeviceRes             ;
   int yDeviceRes             ;
   int firstChar              ;
   int lastChar               ;
   int breakChar              ;
   int defaultChar            ;
}

global %PrinterInfo {
   string name;
   string device;
   Bool   defaultPrinter;
}
