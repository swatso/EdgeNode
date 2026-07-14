; Onboarding RFID path 2
; Used only when the RFID object has not been detected on Path 1
; This is likely to be a smaller object and needs to be moved closer
; to the side antenna. Caution, large objects will crash into the antenna !
M106 S0
G4 S2
G1 X252.000 Y0.000 Z90.000
M106 S255
G4 S2
G1 X252.000 Y18.000 Z90.000
G1 X92.000 Y15.000 Z90.000
G1 X0.000 Y0.000 Z90.000