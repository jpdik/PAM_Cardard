gcc -fPIC -fno-stack-protector -c src/pam_cardard.c
sudo ld -x --shared -o bin/pam_cardard.so pam_cardard.o
rm -r pam_cardard.o