OS/Z Lib C
==========

A legalapvetőbb függvénykönyvtár, ami sztandardizált hozzáférést tesz lehetővé
az operációs rendszer nyújtotta szolgáltatásokhoz, és minden alacsony szintű
rendszerhívást elrejt.
Két részre oszlik:

 - platform független rész
 - platform függő rész

Az ebben a könyvtárban lévő C források a függetlenek. A platform függő kód
alkönyvtárakba lett csoportosítva, és .S Assembly forrásokat is tartalmaznak,
legalább egyet, a crt0.S-t.
