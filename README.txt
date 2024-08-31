nb = 52
envoi requette get data
attente reponse get data
donnes recues:61
Canal 1 Tension V: 231.90
Canal 1 Curent A: 0.28
Canal 1 active power W: 39.61
Canal 1 positive active energy Wh: 0.00
Canal 1 powerfactor: 0.62
Canal 1 negative active energy Wh: 3.00                           // Cette valeur doit augmenter quand il y a de la consomation sur les triacs
Power direction channel 1, 0 positive, 1 negative: 1              // Valeur doit être 1 quand le balon est en route (sinon passer le fil à l'envers)
Power direction channel 2, 0 positive, 1 negative: 1              // Cette valeur doit être égale à 1 quand on tire du courant sur le reseau, à 0 si on injecte
Frequence Hz: 49.97
Canal 2 Tension V: 231.90
Canal 2 Curent A: 0.28
Canal 2 active power W: 39.30
Canal 2 positive active energy Wh: 5.00                           // Energie injectee
Canal 2 powerfactor: 0.60                                        
Canal 2 negative active energy Wh: 3.00                           // Energie consomme
Scanning... Try I2C device found at address 0x3C  !
  OLED adaptor ?
  OLED Device may be: SSD1306 128x32                              // Cette ligne doit s'afficher si l'ecran est bien connecte
I2C device found at address 0x57  !
  EEPROM                                                          // Optionnel si le RTC est branché
I2C device found at address 0x68  !
  Real-Time Clock (RTC): DS3231                                   // Optionnel si le RTC est branché
done I2C scan
Ecran Trouvé : OK                                                 // ou bien : Erreur: pas d'écran
RTC Trouvé : OK                                                   // ou bien : Warning: pas de RTC                                            