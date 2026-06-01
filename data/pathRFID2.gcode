; Onboarding RFID path 2
; Used only when the RFID object has not been detected on Path 1
; This is likely to be a smaller object and needs to be moved closer
; to the side antenna. Caution, large objects will crash into the antenna !
M106 S255
G4 S2
G1 Y15.0 F500
G1 X100.000 Y10.000 Z90.000 F500
G1 X00.000 Y0.000, Z90.000 F500
M106 S0
G4 S2
