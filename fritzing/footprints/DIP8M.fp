
	
# retain backwards compatibility to older versions of PKG_DIL 
# which did not have 100,60,28 args

        
              
        
              
        
              
	
	
	
Element(0x00 "Dual in-line package, medium wide (400 mil)" "" "DIP8M" 270 100 3 100 0x00)
(
	Pin(50 50 60 28 "1" 0x101)
	Pin(50 150 60 28 "2" 0x01)
	Pin(50 250 60 28 "3" 0x01)
	Pin(50 350 60 28 "4" 0x01)
	
	Pin(450 350 60 28 "5" 0x01)
	Pin(450 250 60 28 "6" 0x01)
	Pin(450 150 60 28 "7" 0x01)
	Pin(450 50 60 28 "8" 0x01)
	
	ElementLine(0 0 0 400 10)
	ElementLine(0 400 500 400 10)
	ElementLine(500 400 500 0 10)
	ElementLine(0 0 200 0 10)
	ElementLine(300 0 500 0 10)
	ElementArc(250 0 50 50 0 180 10)
	Mark(50 50)
)

