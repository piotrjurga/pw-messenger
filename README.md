#Komunikator
Program stworzony w ramach przedmiotu Programowanie Współbieżne.
Zawiera serwer i klienta dla prostego komunikatora, z możliwością
rejestracji użytkownika, subskrypcji danego typu wiadomości i rozgłaszania
wiadomości wybranego typu do pozostałych subskrybentów.
Komunikacja odbywa się z wykorzystaniem kolejek wiadomości POSIX.

##Kompilacja
W celu kompilacji projektu należy wykonać skrypt 'build'.
```bash
$ ./build
```
Skrypt utworzy dwa pliki: 'client' i 'server', będące odpowiednio
programami klienta i serwera. Program serwera nie wymaga żadnej
interakcji, natomiast program klienta posiada prosty interfejs ncurses.

##Pliki w projekcie
- PROTOCOL - Opis wykorzystanego protokołu.
- inf136728\_s.c - Kod programu serwera.
- inf136728\_client.h - Plik zawiera funkcje wykorzystywane do komunikacji z serwerem oraz testowy interfejs aplikacij klienta wykorzystujący linię komend zamiast biblioteki ncurses.
- inf136728\_k.c - Kod programu klienta. Wykorzystuje bibliotekę ncurses.
- inf136728\_common.h - Drobne funkcje wykorzystywane w obu programach oraz definicja struktur Message i Subscription.
- inf136728\_stb.h - Biblioteka rozszerzająca standardową bibliotekę C, autorstwa Seana Barretta. Wykorzystana w tym projekcie ze względu na implementację dynamicznej tabeli, oraz wyrażeń regularnych.
